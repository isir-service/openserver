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
#include "base/opmem.h"
#include "pcap.h"
#include "config.h"
#include "iniparser.h"
#include "eth.h"
#include "skb_buff.h"
#include "proto_decode.h"

#define DPDK_PACKET_MAX_SIZE 65535
#define OPDPDK_INTERFACE "opdpdk:interface"

struct dpdk_thread {
	pthread_t recv_thread_id;
	pthread_attr_t recv_thread_attr;
};

struct opdpdk_info {
	char dev[64];
	pcap_t *pcap_handle;
	struct dpdk_thread thread;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
};

static struct opdpdk_info *self = NULL;

static void opdpdk_packet_process(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
	struct op_skb_buff *skb = NULL;
	struct eth_proto_decode *eth = NULL;
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
	if (eth_decode(skb) < 0)
		goto out;

	eth = get_eth_decode(skb->eth_type);
	if (!eth)
		goto out;

	if (eth->cb(skb) < 0)
		goto out;

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

int main(int argc, char* argv[])
{
	struct opdpdk_info *dpdk = NULL;
	char errbuf[256];
	dictionary *dict;
	const char *str;
	//op_daemon();
	opmem_init();
	log_debug_ex("opdpdk init\n");
	dpdk = op_calloc(1, sizeof(struct opdpdk_info));
	if (!dpdk) {
		log_error_ex("opdpdk calloc failed\n");
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
	
	strlcpy(dpdk->dev, str, sizeof(dpdk->dev));
	iniparser_freedict(dict);
	
	log_debug_ex("opdpdk interface:%s\n", dpdk->dev);

	dpdk->pcap_handle = pcap_open_live(dpdk->dev, DPDK_PACKET_MAX_SIZE, 1, 0, errbuf);
	if (!dpdk->pcap_handle) {
		log_error_ex("pcap_open_live failed[%s]\n", errbuf);
		goto exit;
	}

	self = dpdk;

	pcap_loop(dpdk->pcap_handle, -1, opdpdk_packet_process, (u_char *)dpdk);
	return 0;
exit:
	opdpdk_exit(dpdk);
	return -1;

}

