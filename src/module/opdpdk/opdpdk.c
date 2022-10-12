#include <stddef.h>
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
#include "suricate_main.h"
#include "base/oprpc.h"
#include "opbox/oplist.h"

#define OPDPDK_INTERFACE "opdpdk:interface"
#define OPDPDK_SURICATE_YAML "opdpdk:suricata_yaml"

struct suricata_thread
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
};

struct opdpdk_info {
	char dev[64];
	char suri_yaml[256];
	struct event_base *base;
	struct suricata_thread suricata;
	void *mem;
};

static struct opdpdk_info *self = NULL;

void opdpdk_exit(void *dpdk)
{
	log_debug_ex("opdpdk exit\n");

	if (!dpdk)
		return;

	return;
}

static void *suricate_routine (void *arg)
{
#define OPDPDK_ARGV_NUM 4
#define OPDPDK_ARGV_ELE_LENGTH 128
struct opdpdk_info *dpdk = (struct opdpdk_info *)arg;

	int i;
	int argc;
	char **argv = op_calloc(1, sizeof(char*) * (OPDPDK_ARGV_NUM+1));
	if (!argv) {
		log_error_ex ("op_calloc failed\n");
		goto out;
	}

	argv[OPDPDK_ARGV_NUM] = NULL;
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

int main(int argc, char* argv[])
{
	struct opdpdk_info *dpdk = NULL;
	dictionary *dict;
	const char *str;
	void *mem = NULL;

	op_daemon();
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

	if(!(str = iniparser_getstring(dict, OPDPDK_SURICATE_YAML,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", OPDPDK_SURICATE_YAML);
		iniparser_freedict(dict);
		goto exit;
	}

	op_strlcpy(dpdk->suri_yaml, str, sizeof(dpdk->suri_yaml));
	iniparser_freedict(dict);
	
	log_debug_ex("opdpdk interface:%s\n", dpdk->dev);

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

