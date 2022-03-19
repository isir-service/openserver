#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
 #include <linux/if_packet.h>
 #include <net/ethernet.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "opdpdk.h"
#include "base/oplog.h"
#include "opbox/utils.h"
#include "base/hash_mem.h"
#include "base/opmem.h"
#include "pcap.h"
#include "config.h"
#include "iniparser.h"
#include "eth.h"
#include "skb_buff.h"
#include "proto_decode.h"
#include "suricate_main.h"
#include "base/oprpc.h"
#include "opbox/oplist.h"

#define DPDK_PACKET_MAX_SIZE 65535
#define OPDPDK_INTERFACE "opdpdk:interface"
#define OPDPDK_SURICATE_YAML "opdpdk:suricata_yaml"
#define OPDPDK_WORK_THREAD_NUM "opdpdk:work_thread_num"
#define OPDPDK__MAXTHNREAD_NUM 256

#define OPDPDK_MAX_LENGTH_PKT 2048

struct dpdk_pcap_thread {
	pthread_t thread_id;
	pthread_attr_t thread_attr;
};

struct suricata_thread {
	pthread_t thread_id;
	pthread_attr_t thread_attr;
};

struct opdpdk_info {
	char dev[64];
	char suri_yaml[256];
	pcap_t *pcap_handle;
	struct dpdk_pcap_thread pcap_thread;
	struct dpdk_work_thread *work_thread;
	struct event_base *base;
	struct suricata_thread suricata;
	unsigned int work_thread_num;
	struct event *opmem_watchd;
	void *mem;
};

struct pkt_raw
{
	void *raw;
	unsigned int raw_len;
	struct oplist_head list;
};

static struct opdpdk_info *self = NULL;

static void opdpdk_packet_process(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
	struct op_skb_buff *skb = NULL;
	struct eth_proto_decode *eth = NULL;
	unsigned int queueid = 0;
	struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;
	struct pkt_raw *pkt = NULL;
	void *raw = NULL;

	if (pkthdr->len <= 0 || pkthdr->len > OPDPDK_MAX_LENGTH_PKT)
	{
		log_warn_ex("pkthdr len is error:%u\n", (unsigned int)pkthdr->len);
		goto out;
	}

	skb = op_calloc(1, sizeof(struct op_skb_buff));
	if (!skb) {
		log_warn_ex("skb buff calloc failed\n");
		goto out;
	}

	skb->frame = (u_char*)packet;
	skb->frame_len = pkthdr->len;
	skb->ts = pkthdr->ts;

//l2 layer
	if (eth_decode(skb) < 0){
		log_warn_ex("eth_decode failed\n");
		goto out;
	}

	eth = get_eth_decode(skb->eth_type);
	if (!eth || !eth->cb) {
		//log_warn_ex("not support eth type:%x\n", skb->eth_type);
		goto out;
	}

//l3 layer
	if (!eth->cb(skb, 1))
		goto out;

	if (skb->iphdr ==NULL)
		queueid = dpdk->work_thread_num-1;
	else {
		if (dpdk->work_thread_num == 1)
			queueid = 0;
		else
			queueid = (skb->iphdr->saddr+skb->iphdr->daddr) % (dpdk->work_thread_num-1);
	}

	pkt = op_calloc(1, sizeof(struct pkt_raw));
	if (!pkt) {
		log_warn_ex("op_calloc pkt raw failed\n");
		goto out;
	}

	raw = op_calloc(1, pkthdr->len);
	if (!raw) {
		log_warn_ex("op_calloc raw failed\n");
		op_free(pkt);
		goto out;
	}

	memcpy(raw, packet, pkthdr->len);
	pkt->raw = raw;
	pkt->raw_len = pkthdr->len;
	
	pthread_mutex_lock(&dpdk->work_thread[queueid].lock);
	oplist_add_tail(&pkt->list, &dpdk->work_thread[queueid].pkt_list);
	pthread_cond_broadcast(&dpdk->work_thread[queueid].cont);
	pthread_mutex_unlock(&dpdk->work_thread[queueid].lock);
out:
	if (skb)
		op_free(skb);

	return;
}

void opdpdk_exit(void *dpdk)
{
	log_debug_ex("opdpdk exit\n");

	if (!dpdk)
		return;

	return;
}

static unsigned long frag_skb_hash (const void *skb)
{
	struct op_skb_buff *skb_buff = (struct op_skb_buff *)skb;
	return skb_buff->iphdr->saddr + skb_buff->iphdr->saddr;
}

static int frag_skb_compare(const void *src_node, const void *dest_node)
{
	struct op_skb_buff *skb_src = (struct op_skb_buff *)src_node;
	struct op_skb_buff *skb_dst = (struct op_skb_buff *)dest_node;
	return !(skb_src->iphdr->saddr == skb_dst->iphdr->saddr && skb_src->iphdr->daddr == skb_dst->iphdr->daddr &&
			skb_src->iphdr->id == skb_dst->iphdr->id);
}

static void *opdpdk_pcap_routine (void *arg)
{
	
	struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;
	if (!dpdk)
		goto out;
	pcap_loop(dpdk->pcap_handle, -1, opdpdk_packet_process, (u_char *)dpdk);
	log_info_ex("opdpdk_packet_process exit\n");
out:
	log_info_ex("opdpdk pcap routine exit\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}

static void *opdpdk_work_routine (void *arg)
{
	struct dpdk_work_thread *work_thread = (struct dpdk_work_thread *)arg;
	if (!work_thread)
		goto out;

	struct pkt_raw *pkt;
	struct op_skb_buff *skb;
	struct op_skb_buff *skb_tmp;
	struct eth_proto_decode *eth;

	work_thread->run = 1;
	while(work_thread->run) {
		pthread_mutex_lock(&work_thread->lock);
		if (oplist_empty(&work_thread->pkt_list))
			pthread_cond_wait(&work_thread->cont, &work_thread->lock);

		if(oplist_empty(&work_thread->pkt_list)) {
			pthread_mutex_unlock(&work_thread->lock);
			continue;
		}

		pkt = oplist_first_entry(&work_thread->pkt_list, struct pkt_raw , list);
		if (!pkt) {
			log_info_ex ("list_first_entry failed\n");
			pthread_mutex_unlock(&work_thread->lock);
			continue;
		}

		oplist_del_init(&pkt->list);
		pthread_mutex_unlock(&work_thread->lock);
		// work
		skb = op_calloc(1, sizeof(struct op_skb_buff));
		if (!skb) {
			log_warn_ex("skb buff calloc failed\n");
			op_free(pkt->raw);
			op_free(pkt);
			continue;
		}

		skb->frame = pkt->raw;
		skb->frame_len = pkt->raw_len;
		skb->work_thread = work_thread;
		op_free(pkt);

		//l2 layer
		if (eth_decode(skb) < 0){
			log_warn_ex("eth_decode failed\n");
			goto free_next;
		}
	
		eth = get_eth_decode(skb->eth_type);
		if (!eth || !eth->cb)
			goto free_next;

		//l3 layer
		skb_tmp = eth->cb(skb, 0);
		if (!skb_tmp)
			goto free_next;

		if (skb_tmp->ip_frag.frag) {
			if (!skb_tmp->ip_frag.first_find)
				goto free_next;

			if (!skb_tmp->ip_frag.over)
				goto next_pkt;
		}

		//l4
		switch(skb_tmp->eth_type) {
			case OP_ETHTYPE_IP:
				if (skb_tmp->ip_frag.frag)
					goto next_pkt;
				break;
			default:
				break;
		}

	free_next:
		if (skb && skb->frame)
			op_free(skb->frame);
		if (skb)
			op_free(skb);
	next_pkt:
		;
	}
out:
	log_info_ex("opdpdk_work_routine exit\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}


static void *suricate_routine (void *arg)
{
#define OPDPDK_ARGV_NUM 4
#define OPDPDK_ARGV_ELE_LENGTH 128
struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;

	int i;
	int argc;
	char **argv = op_calloc(1, sizeof(char*) * OPDPDK_ARGV_NUM);
	if (!argv) {
		log_error_ex ("op_calloc failed\n");
		goto out;
	}

	for(i = 0; i < OPDPDK_ARGV_NUM; i++) {
		argv[i] = op_calloc(1, OPDPDK_ARGV_ELE_LENGTH);
		if (!argv[i]) {
			log_error_ex ("op_calloc failed, index=%d\n",i);
			goto out;
		}
	}

	op_strlcpy(argv[0],"suricata", OPDPDK_ARGV_ELE_LENGTH);
	snprintf(argv[1], OPDPDK_ARGV_ELE_LENGTH, "--af-packet=%s", dpdk->dev);
	op_strlcpy(argv[2],"-c", OPDPDK_ARGV_ELE_LENGTH);
	op_strlcpy(argv[3],dpdk->suri_yaml, OPDPDK_ARGV_ELE_LENGTH);
	argc = OPDPDK_ARGV_NUM;

	suriacte_run(argc, argv);

out:
	log_warn_ex("suricata routing exit\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}

void ip_frag_watch (void *skb)
{
	struct op_skb_buff *skb_tmp = skb;
	struct ip_frag_timeout *node = NULL;
	struct dpdk_work_thread *work_thread = (struct dpdk_work_thread *)skb_tmp->work_thread;
	time_t t = time(NULL);
	if (!skb)
		return;

	if (t - skb_tmp->ip_frag.time  <= IP_FRAG_TIMEOUT)
		return;

	node = op_calloc(1, sizeof(struct ip_frag_timeout));
	if (!node)
		return;

	node->skb = skb_tmp;
	oplist_add_tail(&node->list, &work_thread->timeout_frag_list);
	return;
}

static void opdpdk_ipfrag_watchd(evutil_socket_t fd , short what, void *arg)
{
	struct dpdk_work_thread *work_thread = (struct dpdk_work_thread *)arg;
	struct timeval tv;
	struct ip_frag_timeout *node = NULL;
	struct ip_frag_timeout *node_tmp = NULL;
	struct ip_frag_data *data_head;
	struct ip_frag_data *data_head_tmp;

	pthread_mutex_lock(&work_thread->lock);
	op_hash_mem_doall(work_thread->ip_frag_skb_hash, ip_frag_watch);
	oplist_for_each_entry_safe(node, node_tmp, &work_thread->timeout_frag_list, list) {
		data_head = node->skb->ip_frag.data_head;
		while(data_head) {
			data_head_tmp = data_head->next;
			op_free(data_head);
			data_head = data_head_tmp;
		}
		oplist_del(&node->list);
		log_warn_ex("proto:%hhu, seq=%hu timeout\n", node->skb->iphdr->protocol, ntohs(node->skb->iphdr->id));
		op_hash_mem_delete(work_thread->ip_frag_skb_hash, node->skb);
		op_free(node->skb);
		op_free(node);
	}

	pthread_mutex_unlock(&work_thread->lock);
	tv.tv_sec = IP_FRAG_TIMEOUT_CHECK_S;
	tv.tv_usec = 0;
	event_add(work_thread->ipfrag_watchd, &tv);
	return;
}

int opdpdk_get_mem_pool_infrmation(unsigned char *req, int req_size, unsigned char *response, int res_size)
{
	int src_size = 0;
	src_size = op_mem_information((char*)response, res_size);

	if (src_size > res_size)
		log_warn_ex("mem pool, message truncate[src_size=%d, res_size=%d]\n", src_size, res_size);

	return src_size;
}

int opdpdk_get_mem_pool_father_node_infrmation(unsigned char *req, int req_size, unsigned char *response, int res_size)
{
	int src_size = 0;
	src_size = op_mem_father_node_information((char*)response, res_size);

	if (src_size > res_size)
		log_warn("mem pool, message truncate[src_size=%d, res_size=%d]\n", src_size, res_size);

	return src_size;
}


void opdpdk_rpc_register(void)
{
	op_tipc_register(tipc_opdpdk_show_mem_poll,opdpdk_get_mem_pool_infrmation);
	
	op_tipc_register(tipc_opdpdk_show_mem_poll_father_node,opdpdk_get_mem_pool_father_node_infrmation);
	return;
}


static void opdpdk_opmem_watchd(evutil_socket_t fd , short what, void *arg)
{
	struct timeval tv;

	tv.tv_sec = 60;
	tv.tv_usec = 0;
	op_mem_release_check(arg);
	event_add(self->opmem_watchd, &tv);
	return;
}

int main(int argc, char* argv[])
{
	struct opdpdk_info *dpdk = NULL;
	char errbuf[256];
	dictionary *dict;
	const char *str;
	unsigned int i = 0;
	struct timeval tv;
	void *mem = NULL;

	//op_daemon();
	mem = opmem_init();
	if (!mem) {
		log_warn_ex("pmem failed\n");
		goto exit;
	}

	op_tipc_init(rpc_tipc_module_opdpdk);
	opdpdk_rpc_register();
	log_debug_ex("opdpdk init\n");
	dpdk = op_calloc(1, sizeof(struct opdpdk_info));
	if (!dpdk) {
		log_error_ex("opdpdk calloc failed\n");
		goto exit;
	}

	dpdk->mem = mem;
	self = dpdk;

	dpdk->base = event_base_new();
	if (!dpdk->base) {
		log_error ("event_base_new faild\n");
		goto exit;
	}

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		log_error_ex ("iniparser_load faild[%s]\n",OPSERVER_CONF);
		goto exit;
	}

	if(!(str = iniparser_getstring(dict, OPDPDK_INTERFACE,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", OPDPDK_INTERFACE);
		iniparser_freedict(dict);
		goto exit;
	}

	op_strlcpy(dpdk->dev, str, sizeof(dpdk->dev));

	dpdk->work_thread_num = (unsigned int)iniparser_getint(dict, OPDPDK_WORK_THREAD_NUM,4);
	if (dpdk->work_thread_num > OPDPDK__MAXTHNREAD_NUM || dpdk->work_thread_num <= 0)
	{
		log_error_ex ("dpdk work thread num max supoport num: %u, check your config\n", OPDPDK__MAXTHNREAD_NUM);
		iniparser_freedict(dict);
		goto exit;
	}

	log_debug_ex("opdpdk work_thread_num:%u\n", dpdk->work_thread_num);

	dpdk->work_thread = op_calloc(1, sizeof(struct dpdk_work_thread) * dpdk->work_thread_num);
	if (!dpdk->work_thread) {
		log_error_ex ("dpdk work thread calloc failed\n");
		iniparser_freedict(dict);
		goto exit;
	}

	if(!(str = iniparser_getstring(dict, OPDPDK_SURICATE_YAML,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", OPDPDK_SURICATE_YAML);
		iniparser_freedict(dict);
		goto exit;
	}

	op_strlcpy(dpdk->suri_yaml, str, sizeof(dpdk->suri_yaml));
	iniparser_freedict(dict);
	
	log_debug_ex("opdpdk interface:%s\n", dpdk->dev);

	dpdk->pcap_handle = pcap_open_live(dpdk->dev, DPDK_PACKET_MAX_SIZE, 1, 100, errbuf);
	if (!dpdk->pcap_handle) {
		log_error_ex("pcap_open_live failed[%s]\n", errbuf);
		goto exit;
	}

	if(pthread_attr_init(&dpdk->pcap_thread.thread_attr)) {
		log_error_ex ("pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&dpdk->pcap_thread.thread_id, &dpdk->pcap_thread.thread_attr, opdpdk_pcap_routine, dpdk)) {
		log_error_ex ("pthread_create faild\n");
		goto exit;
	}

	for (i = 0; i < dpdk->work_thread_num; i++) {
		dpdk->work_thread[i].ip_frag_skb_hash = op_hash_mem_new(frag_skb_hash, frag_skb_compare);
		if (!dpdk->work_thread[i].ip_frag_skb_hash)
		{
			log_error_ex ("op_hash_mem_new faild, id=%u\n", i);
			goto exit;
		}

		dpdk->work_thread[i].ipfrag_watchd = evtimer_new(dpdk->base, opdpdk_ipfrag_watchd, &dpdk->work_thread[i]);
		if (!dpdk->work_thread[i].ipfrag_watchd) {
			log_warn_ex("ipfrag watchd failed, id=%u\n", i);
			goto exit;
		}

		tv.tv_sec = IP_FRAG_TIMEOUT_CHECK_S;
		tv.tv_usec = 0;
		event_add(dpdk->work_thread[i].ipfrag_watchd, &tv);
		
		OPINIT_LIST_HEAD(&dpdk->work_thread[i].pkt_list);
		OPINIT_LIST_HEAD(&dpdk->work_thread[i].timeout_frag_list);

		if(pthread_condattr_init(&dpdk->work_thread[i].cont_attr)) {
			log_error_ex ("pthread_condattr_init faild id=%u\n", i);
			goto exit;
		}
		
		if(pthread_cond_init(&dpdk->work_thread[i].cont, &dpdk->work_thread[i].cont_attr)) {
			log_error_ex ("pthread_cond_init faild id=%u\n", i);
			goto exit;
		}

		if(pthread_mutexattr_init(&dpdk->work_thread[i].attr)) {
			log_error ("pthread_mutexattr_init faild, id=%u\n", i);
			goto exit;
		}
		
		if(pthread_mutex_init(&dpdk->work_thread[i].lock, &dpdk->work_thread[i].attr)) {
			log_error ("pthread_mutex_init faild, id=%u\n", i);
			goto exit;
		}

		if(pthread_attr_init(&dpdk->work_thread[i].thread_attr)) {
			log_error_ex ("pthread_attr_init faild, id=%u\n", i);
			goto exit;
		}
		
		if(pthread_create(&dpdk->work_thread[i].thread_id, &dpdk->work_thread[i].thread_attr, opdpdk_work_routine, &dpdk->work_thread[i])) {
			log_error_ex ("pthread_create faild, id=%u\n",i);
			goto exit;
		}
	}

	if(pthread_attr_init(&dpdk->suricata.thread_attr)) {
		log_error_ex ("pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&dpdk->suricata.thread_id, &dpdk->suricata.thread_attr, suricate_routine, dpdk)) {
		log_error_ex ("pthread_create faild\n");
		goto exit;
	}

	dpdk->opmem_watchd = evtimer_new(dpdk->base, opdpdk_opmem_watchd, dpdk->mem);
	if (!dpdk->opmem_watchd) {
		log_warn_ex("process opmem failed\n");
		goto exit;
	}

	tv.tv_sec = 60;
	tv.tv_usec = 0;
	event_add(dpdk->opmem_watchd, &tv);

	if(event_base_loop(dpdk->base, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		log_error ("opdpdk failed\n");
		goto exit;
	}

	return 0;

exit:
	opdpdk_exit(dpdk);
	return -1;

}

