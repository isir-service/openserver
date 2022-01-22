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
#include "event.h"

#include "base/opmem.h"
#include "pcap.h"
#include "config.h"
#include "iniparser.h"
#include "eth.h"
#include "skb_buff.h"
#include "proto_decode.h"
#include "suricate_main.h"

#define DPDK_PACKET_MAX_SIZE 65535
#define OPDPDK_INTERFACE "opdpdk:interface"
#define OPDPDK_SURICATE_YAML "opdpdk:suricata_yaml"

struct dpdk_thread {
	void *skb_hash;
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	pthread_mutex_t lock;
	pthread_mutexattr_t lock_attr;
};

struct suricata_thread {
	pthread_t thread_id;
	pthread_attr_t thread_attr;
};

struct opdpdk_info {
	char dev[64];
	char suri_yaml[256];
	pcap_t *pcap_handle;
	struct dpdk_thread thread;
	struct event_base *base;
	struct event *ipfrag_watchd;
	struct suricata_thread suricata;
};

static struct opdpdk_info *self = NULL;

static void opdpdk_packet_process(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
	struct op_skb_buff *skb = NULL;
	struct op_skb_buff *skb_tmp = NULL;
	struct eth_proto_decode *eth = NULL;
	int frag_ret= 0;

	if (pkthdr->len <= 0)
	{
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
	if (eth_decode(skb) < 0)
		goto out;

	eth = get_eth_decode(skb->eth_type);
	if (!eth || !eth->cb)
		goto out;

	if (eth->cb(skb) < 0)
		goto out;

	if (skb->eth_type == OP_ETHTYPE_IP) {
		if ((skb->ip_frag.ip_mf == 0 && skb->ip_frag.ip_frag_offset > 0) || skb->ip_frag.ip_mf == 1) {
			pthread_mutex_lock(&self->thread.lock);
			skb_tmp = op_hash_mem_retrieve(self->thread.skb_hash, skb);
			if (!skb_tmp) {
				op_hash_mem_insert(self->thread.skb_hash, skb);
				pthread_mutex_unlock(&self->thread.lock);
				return;
			}

			frag_ret = ip_frag(skb_tmp, skb);
			if (frag_ret < 0) {
				pthread_mutex_unlock(&self->thread.lock);
				goto out;
			}

			if (frag_ret == IP_FRAG_CONTINUE) {
				pthread_mutex_unlock(&self->thread.lock);
				goto out;
			}
			op_free(skb);
			op_hash_mem_delete(self->thread.skb_hash, skb_tmp);
			skb = skb_tmp;
			pthread_mutex_unlock(&self->thread.lock);
		}
	}

	
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

static unsigned long skb_hash (const void *skb)
{
	struct op_skb_buff *skb_buff = (struct op_skb_buff *)skb;
	return skb_buff->iphdr->saddr + skb_buff->iphdr->saddr;
}

static int skb_compare(const void *src_node, const void *dest_node)
{
	struct op_skb_buff *skb_src = (struct op_skb_buff *)src_node;
	struct op_skb_buff *skb_dst = (struct op_skb_buff *)dest_node;
	return !(skb_src->iphdr->saddr == skb_dst->iphdr->saddr && skb_src->iphdr->daddr == skb_dst->iphdr->daddr);
}

static void *opdpdk_routine (void *arg)
{
	
	struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;
	pcap_loop(dpdk->pcap_handle, -1, opdpdk_packet_process, (u_char *)dpdk);

	log_info_ex("opdpdk_routine exit\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}

static void *suricate_routine (void *arg)
{
#define OPDPDK_ARGV_NUM 4
#define OPDPDK_ARGV_ELE_LENGTH 128
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
	snprintf(argv[1], OPDPDK_ARGV_ELE_LENGTH, "--af-packet=%s", self->dev);
	op_strlcpy(argv[2],"-c", OPDPDK_ARGV_ELE_LENGTH);
	op_strlcpy(argv[3],self->suri_yaml, OPDPDK_ARGV_ELE_LENGTH);
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
	if (!skb)
		return;

	op_free(skb);
	return;
}

static void opdpdk_ipfrag_watchd(evutil_socket_t fd , short what, void *arg)
{
	struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;
	struct timeval tv;

	pthread_mutex_lock(&dpdk->thread.lock);
	op_hash_mem_doall(dpdk->thread.skb_hash, ip_frag_watch);
	pthread_mutex_unlock(&dpdk->thread.lock);
	tv.tv_sec = IP_FRAG_TIMEOUT_S;
	tv.tv_usec = 0;
	event_add(dpdk->ipfrag_watchd, &tv);
	return;
}

int main(int argc, char* argv[])
{
	struct opdpdk_info *dpdk = NULL;
	char errbuf[256];
	dictionary *dict;
	const char *str;
	op_daemon();
	opmem_init();
	log_debug_ex("opdpdk init\n");
	dpdk = op_calloc(1, sizeof(struct opdpdk_info));
	if (!dpdk) {
		log_error_ex("opdpdk calloc failed\n");
		goto exit;
	}

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
	
	if(!(str = iniparser_getstring(dict, OPDPDK_SURICATE_YAML,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", OPDPDK_SURICATE_YAML);
		iniparser_freedict(dict);
		goto exit;
	}

	op_strlcpy(dpdk->suri_yaml, str, sizeof(dpdk->suri_yaml));
	iniparser_freedict(dict);
	
	log_debug_ex("opdpdk interface:%s\n", dpdk->dev);

	dpdk->pcap_handle = pcap_open_live(dpdk->dev, DPDK_PACKET_MAX_SIZE, 1, 0, errbuf);
	if (!dpdk->pcap_handle) {
		log_error_ex("pcap_open_live failed[%s]\n", errbuf);
		goto exit;
	}

	dpdk->thread.skb_hash = op_hash_mem_new(skb_hash, skb_compare);
	if (!dpdk->thread.skb_hash)
	{
		log_error_ex ("op_hash_mem_new faild\n");
		goto exit;
	}

	if(pthread_mutexattr_init(&dpdk->thread.lock_attr)) {
		log_error_ex ("pthread_mutexattr_init faild\n");
		goto exit;
	}

	if(pthread_mutex_init(&dpdk->thread.lock, &dpdk->thread.lock_attr)) {
		log_error_ex ("pthread_mutex_init faild\n");
		goto exit;
	}

	if(pthread_attr_init(&dpdk->thread.thread_attr)) {
		log_error_ex ("pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&dpdk->thread.thread_id, &dpdk->thread.thread_attr, opdpdk_routine, dpdk)) {
		log_error_ex ("pthread_create faild\n");
		goto exit;
	}
	

	dpdk->ipfrag_watchd = evtimer_new(dpdk->base, opdpdk_ipfrag_watchd, dpdk);
	if (!dpdk->ipfrag_watchd) {
		log_warn_ex("ipfrag watchd failed\n");
		goto exit;
	}

	if(pthread_attr_init(&dpdk->suricata.thread_attr)) {
		log_error_ex ("pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&dpdk->suricata.thread_id, &dpdk->suricata.thread_attr, suricate_routine, dpdk)) {
		log_error_ex ("pthread_create faild\n");
		goto exit;
	}

	if(event_base_loop(dpdk->base, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		log_error ("opdpdk failed\n");
		goto exit;
	}

	return 0;

exit:
	opdpdk_exit(dpdk);
	return -1;

}

