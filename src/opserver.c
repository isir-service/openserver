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

struct _opserver_struct_ {
	void *log;
	void *bus;
	void *cli;
	void *mgr;
	void *_4g;
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

	oplog_exit(_op->_4g);
	opmgr_exit(_op->mgr);
	opcli_exit(_op->cli);
	opcli_exit(_op->bus);
	oplog_exit(_op->log);

	free(_op);
	return;
}

int main(int argc, char*argv[])
{
	struct _opserver_struct_ *_op = NULL;

	signal(SIGUSR1, signal_handle);
	signal(SIGPIPE, SIG_IGN);

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
