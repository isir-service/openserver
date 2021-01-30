#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "opbox/usock.h"
#include "base/oplog.h"
#include "base/opbus.h"
#include "base/opcli.h"
#include "opmgr.h"
#include "op4g.h"
#include "event.h"
#include "base/opsql.h"
#include "config.h"
#include "base/opsql.h"
#include "timer_service.h"
#include "iniparser.h"
#include "spider.h"
#include "webserver.h"

#define OPSERVER_PATH "env:path"
#define OPSERVER_LIB "env:lib"

struct _opserver_struct_ {
	void *log;
	void *bus;
	void *cli;
	void *mgr;
	void *_4g;
	void *sql;
	void *timer_service;
	void *spider;
	void *web;
	struct event_base *base;
};

struct _opserver_struct_ *self = NULL;

int opserver_init(void)
{
	return 0;
}

void signal_handle(int signal_no)
{
	return;
}
void opserver_exit(struct _opserver_struct_ *_op)
{
	if (_op)
		return ;

	printf("opserver_exit\n");
	timer_service_exit(_op->timer_service);
	oplog_exit(_op->_4g);
	opmgr_exit(_op->mgr);
	opsql_exit(_op->sql);
	opcli_exit(_op->cli);
	opcli_exit(_op->bus);
	oplog_exit(_op->log);
	spider_exit(_op->spider);
	webserver_exit(_op->web);
	free(_op);
	return;
}

int opserver_env_set(void)
{
	dictionary *dict;
	const char *str;
	char buf[2048] = {};
	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FILE__,__LINE__, OPSERVER_CONF);
		goto out;
	}

	if(!(str = iniparser_getstring(dict,OPSERVER_PATH,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, OPSERVER_PATH);
		iniparser_freedict(dict);
		goto out;
	}

	snprintf(buf, sizeof(buf), "%s:%s", getenv("PATH"), str);
	
	printf ("path=%s\n", buf);
	setenv("PATH", buf, 1);

	if(!(str = iniparser_getstring(dict,OPSERVER_LIB,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, OPSERVER_LIB);
		iniparser_freedict(dict);
		goto out;
	}
	
	snprintf(buf, sizeof(buf), "%s:%s", getenv("LD_LIBRARY_PATH"), str);
	iniparser_freedict(dict);

	printf ("lib=%s\n", buf);

	setenv("LD_LIBRARY_PATH", buf, 1);
	
	return 0;
out:
	return -1;
}

int main(int argc, char*argv[])
{
	daemon(1,0);
	struct _opserver_struct_ *_op = NULL;
	signal(SIGUSR1, signal_handle);
	signal(SIGPIPE, SIG_IGN);
	srand(time(NULL));
	if (opserver_env_set() < 0) {
		printf("opserver env set failed\n");
		goto exit;
	}

	_op = calloc(1, sizeof(struct _opserver_struct_));
	if (!_op) {
		printf("opserver calloc failed\n");
		goto exit;
	}

	_op->log = oplog_init();
	if (!_op->log) {
		printf("opserver oplog failed\n");
		goto exit;
	}

	_op->bus = opbus_init();
	if (!_op->bus) {
		printf("opserver opbus failed\n");
		goto exit;
	}

	_op->cli = opcli_init();
	if (!_op->cli) {
		printf("opserver opcli failed\n");
		goto exit;
	}

	log_debug("test log\n");
	log_info("test log\n");
	log_warn("test log\n");
	log_error("test log\n");

	_op->sql = opsql_init(OPSERVER_CONF);
	if (!_op->sql) {
		printf("opserver opsql failed\n");
		goto exit;
	}

	_op->mgr = opmgr_init();
	if (!_op->mgr) {
		printf("opserver opmgr failed\n");
		goto exit;
	}

	_op->_4g = op4g_init();
	if (!_op->_4g) {
		printf("opserver op4g failed\n");
		goto exit;
	}
	
	_op->timer_service = timer_service_init();
	if (!_op->timer_service) {
		printf("opserver timer_service_init failed\n");
		goto exit;
	}
	
	_op->spider = spider_init();
	if (!_op->spider) {
		printf("opserver spider init failed\n");
		goto exit;
	}
	
	//_op->web = webserver_init();
	//if (!_op->web) {
		//printf("opserver webserver init failed\n");
		//goto exit;
	//}

	_op->base = event_base_new();
	if (!_op->base) {
		printf ("%s %d opserver event_base_new failed\n",__FILE__,__LINE__);
		goto exit;
	}
	
	self = _op;

	if(event_base_loop(_op->base, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		printf ("%s %d opserver failed\n",__FILE__,__LINE__);
		goto exit;
	}

	return 0;
exit:
	sleep(3);
	opserver_exit(_op);
	return -1;
}
