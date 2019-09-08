#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>

#include <libubox/usock.h>
#include <event2/event.h>
#include <zlog/zlog.h>
#include <libubox/utils.h>
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>


#include "log_job.h"
#include "oplog.h"


#define optstring "hf"
static struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "fork", no_argument, NULL, 'f' }, 
	
	{ 0, 0, 0, 0 }
};

void oplog_usage(void)
{
	fprintf (stdout,"see:\n");
	exit (0);
	return;
}
int num = 0;

void show_memery (void *data)
{
	struct memery_info * info = (struct memery_info *)data;

	if(info == NULL) {
		return;
	}

	printf ("[%d]address:%p,file:%-20s,function:%-20s,line:%-10u,alloc:%-10u\n",num++,info->ptr, info->file, info->function, info->line, info->alloc);

	return;
}

int main(int argc, char *argv[])
{	
	char bind_ip[22] = {};
	char bind_port[10] = {};
	char *pconfig = NULL;
	int c = -1;
	int fork = 0;
	dictionary *dic = NULL;
	const char *pstr = NULL;
	struct log_job *job = NULL;
	int ret = -1;
	char log_gate[40] = {};
	
	while (1) {
		c = getopt_long(argc, argv,optstring,longopts, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {

			case 'f':
				fork = 1;
				break;
			case '?':
			default:
			oplog_usage();
			
		}
	}


	if (fork) {
		op_daemon();
	}


	pconfig = LOG_PATH;


	dic = iniparser_load(pconfig);
	if (dic == NULL) {
		return -1;
	}


	pstr = iniparser_getstring(dic, "server:ip", NULL);
	if (pstr == NULL || isipv4(pstr) < 0 ) {
		return -1;
	}
	snprintf(bind_ip,sizeof(bind_ip), "%s", pstr);


	pstr = iniparser_getstring(dic, "server:port", NULL);
	if (pstr == NULL || isport(pstr) < 0 ) {
		return -1;
	}
	snprintf(bind_port,sizeof(bind_port), "%s", pstr);


	pstr = iniparser_getstring(dic, "log_gate:gate", NULL);
	if (pstr == NULL) {
		return -1;
	}
	snprintf(log_gate,sizeof(log_gate), "%s", pstr);
	
	iniparser_freedict(dic);
	dic = NULL;
	
	job = log_job_init();
	if(job == NULL) {
		return -1;
	}
	set_log_job(job);
	
	snprintf (job->ip, sizeof(job->ip),"%s", bind_ip);
	job->port = (unsigned short)atoi(bind_port);
	snprintf (job->log_gate, sizeof(job->log_gate),"%s", log_gate);
	
	job->sockfd = usock(USOCK_UDP | USOCK_SERVER, bind_ip, bind_port);
	if (job->sockfd < 0) {
		goto free;
	}


	job->base = op_event_base_new();
	if (job->base == NULL) {
		goto free;
	}

	job->event_listen = event_new(job->base, job->sockfd, EV_READ | EV_PERSIST, log_event_read, job);
	if (job->event_listen == NULL) {
		goto free;
	}

	event_add(job->event_listen, NULL);

	/*zlog*/

	ret = zlog_init(ZLOG_PATH);
	if (ret < 0 ) {
		goto free;
	}
	
	
	job->gate = zlog_get_category(job->log_gate);
	if (job->gate == NULL) {
		goto free;
	}

#if 0
	if (pthread_create(&job->pthread_write, NULL , log_write_thread,job) != 0) {
		goto free;
	}
#endif
	event_base_dispatch(job->base);
	
	log_job_exit(job);
	return 0;

free:
	if(dic) {
		iniparser_freedict(dic);
	}

	if(job) {
		log_job_exit(job);
	}
	return -1;

}
