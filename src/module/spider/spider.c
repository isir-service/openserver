#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "spider.h"
#include "event.h"
#include "base/oplog.h"

struct _spider_thread_ 
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
};

struct _spider_struct {
	struct event_base *base;
	struct _spider_thread_ thread;

};

static void *spider_routine (void *arg)
{
	if(event_base_loop(arg, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		log_error ("op4g_routine failed\n");
		pthread_detach(pthread_self());
		pthread_exit(NULL);
		goto exit;
	}

	log_debug ("op4g_routine exit\n");
exit:
	return NULL;
}

void *spider_init(void)
{
	struct _spider_struct *spider = NULL;

	spider = calloc(1, sizeof(*spider));
	if (!spider) {
		log_error("calloc failed[%d]\n", errno);
		goto exit;
	}

	spider->base = event_base_new();
	if (!spider->base) {
		log_error ("event_base_new faild\n");
		goto exit;
	}

	if(pthread_attr_init(&spider->thread.thread_attr)) {
		log_error ("pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&spider->thread.thread_id, &spider->thread.thread_attr, spider_routine, spider->base)) {
		log_error ("pthread_create faild\n");
		goto exit;
	}

	return spider;

exit:
	spider_exit(spider);
	return NULL;
}

void spider_exit(void *spider)
{

	return;
}

