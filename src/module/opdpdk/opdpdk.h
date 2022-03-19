#ifndef __OPDPDK_H__
#define __OPDPDK_H__
#include <pthread.h>
#include "event.h"

#include "opbox/oplist.h"

struct dpdk_work_thread {
	void *ip_frag_skb_hash;
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	struct event *ipfrag_watchd;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;

	pthread_cond_t cont;
	pthread_condattr_t cont_attr;

	struct oplist_head pkt_list;
	struct oplist_head timeout_frag_list;
	int run;
};

#endif
