#include <arpa/inet.h>

#include "ip.h"
#include "opbox/utils.h"
#include "base/oplog.h"
#include "base/opmem.h"
#include "opdpdk.h"
#include "opbox/utils.h"
#include "base/hash_mem.h"

struct op_skb_buff *ip_decode(struct op_skb_buff *skb, int hash)
{
	struct op_skb_buff *skb_tmp = NULL;
	struct dpdk_work_thread *work_thread = NULL;
	struct ip_frag_data *frag_data = NULL;
	unsigned short ip_data_size = 0;
	struct ip_frag_data *data_head;
	unsigned short df;
	unsigned short mf;
	unsigned short off;
	unsigned short len;
	
	if (!skb)
		goto out;

	if (skb->frame_len - skb->eth_len < sizeof(struct op_iphdr))
		goto out;

	skb->iphdr = (struct op_iphdr*)(skb->frame+skb->eth_len);
	if (!skb->iphdr)
		goto out;

	if (skb->iphdr->version != 4)
		goto out; // not support ipv6

	skb->iphdr_len = skb->iphdr->ihl*4;
	if (skb->iphdr_len > IPV4_HEADER_NORMAL_SIZE) {
		skb->ip_option = skb->frame+skb->eth_len+IPV4_HEADER_NORMAL_SIZE;
		skb->ip_option_length = skb->iphdr_len-IPV4_HEADER_NORMAL_SIZE;
	}

	if (hash == 1)
		return skb;

	df = skb->iphdr->frag_off & htons(IP_DF);
	df = ntohs(df);
	if (df != 0) {
		skb->ip_frag.frag = 0;
		return skb;
	}

	mf = skb->iphdr->frag_off & htons(IP_MF);
	mf = mf?1:0;
	off = skb->iphdr->frag_off&htons(IP_OFFSET);
	off = ntohs(off);

	if (mf == 0 && off == 0)
		return skb;

	skb->ip_frag.frag = 1;
	if (!skb->work_thread)
		goto out;

	skb->ip_frag.time = time(NULL);
	work_thread = (struct dpdk_work_thread *)skb->work_thread;

	pthread_mutex_lock(&work_thread->lock);
	skb_tmp = op_hash_mem_retrieve(work_thread->ip_frag_skb_hash, skb);
	pthread_mutex_unlock(&work_thread->lock);

	if (!skb_tmp) {
		skb->ip_frag.over = 0;
		skb->ip_frag.first_find = 1;
		skb->ip_frag.frag_write_size = ntohs(skb->iphdr->tot_len) - skb->iphdr_len;
		frag_data = op_calloc(1, sizeof(struct ip_frag_data));
		if (!frag_data)
			goto out;

		frag_data->next = NULL;

		frag_data->data = op_calloc(1, skb->ip_frag.frag_write_size);
		if (!frag_data->data)
			goto out;

		memcpy(frag_data->data,skb->frame+skb->eth_len+skb->iphdr_len, skb->ip_frag.frag_write_size);
		frag_data->data_len = skb->ip_frag.frag_write_size;
		frag_data->offset = skb->iphdr->frag_off&htons(IP_OFFSET);
		skb->ip_frag.data_head = frag_data;
		skb->ip_frag.data_tail = frag_data;
	
		pthread_mutex_lock(&work_thread->lock);
		op_hash_mem_insert(work_thread->ip_frag_skb_hash, skb);
		pthread_mutex_unlock(&work_thread->lock);
		return skb;
	}

	data_head = skb->ip_frag.data_head;
	while(data_head) {
		if (data_head->offset == off)
			goto out;

		data_head = data_head->next;
	}

	if (!mf && off != 0 && skb_tmp->ip_frag.last_pkt == 0)
	{
		skb_tmp->ip_frag.last_pkt = 1;
		skb_tmp->ip_frag.last_pkt_offlen = off*8;
	}

	len = off*8;
	if (mf && len == 0 && skb_tmp->ip_frag.last_pkt == 1 && 
			skb_tmp->ip_frag.last_pkt_offlen <= skb_tmp->ip_frag.frag_write_size+(ntohs(skb->iphdr->tot_len) - skb->iphdr_len))
		goto over;

	if (mf || (!mf && len > skb_tmp->ip_frag.frag_write_size+(ntohs(skb->iphdr->tot_len) - skb->iphdr_len))) {
		frag_data = op_calloc(1, sizeof(struct ip_frag_data));
		if (!frag_data)
			goto out;

		skb_tmp->ip_frag.first_find = 0;
		frag_data->next = NULL;
		ip_data_size = ntohs(skb->iphdr->tot_len) - skb->iphdr_len;

		frag_data->data = op_calloc(1, ip_data_size);
		if (!frag_data->data)
			goto out;

		memcpy(frag_data->data,skb->frame+skb->eth_len+skb->iphdr_len, ip_data_size);
		frag_data->data_len = ip_data_size;
		
		frag_data->offset = skb->iphdr->frag_off&htons(IP_OFFSET) ;
		skb_tmp->ip_frag.data_tail->next = frag_data;
		skb_tmp->ip_frag.data_tail = frag_data;
		skb_tmp->ip_frag.frag_write_size += ip_data_size;
		return skb_tmp;
	}


over:
	frag_data = op_calloc(1, sizeof(struct ip_frag_data));
	if (!frag_data)
		goto out;

	frag_data->next = NULL;
	ip_data_size = ntohs(skb->iphdr->tot_len) - skb->iphdr_len;

	frag_data->data = op_calloc(1, ip_data_size);
	if (!frag_data->data)
		goto out;

	memcpy(frag_data->data,skb->frame+skb->eth_len+skb->iphdr_len, ip_data_size);
	frag_data->data_len = ip_data_size;
	
	frag_data->offset = skb->iphdr->frag_off&htons(IP_OFFSET);
	skb_tmp->ip_frag.data_tail->next = frag_data;
	skb_tmp->ip_frag.data_tail = frag_data;
	skb_tmp->ip_frag.frag_write_size += ip_data_size;
	skb_tmp->ip_frag.over = 1;
	skb_tmp->ip_frag.first_find = 0;

	log_warn_ex("proto:%hhu, seq=%hu frag over\n", skb_tmp->iphdr->protocol, ntohs(skb_tmp->iphdr->id));

	return skb_tmp;
out:
	if (frag_data && frag_data->data)
		op_free(frag_data->data);

	if (frag_data)
		op_free(frag_data);

	return NULL;
}

int ip_frag(struct op_skb_buff *skb, struct op_skb_buff *skb_new)
{
	return 0;
}


