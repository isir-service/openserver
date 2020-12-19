#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "opbox/usock.h"
#include "base/oplog.h"
#include "event.h"
#include <signal.h>
struct _opserver_struct_ {
	void *log;
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
	return;
}

int main(int argc, char*argv[])
{
	struct _opserver_struct_ *_op = NULL;

	signal(SIGUSR1, signal_handle);
	printf("helloworld\n");
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
	_op->base = event_base_new();
	if (!_op->base) {
		printf ("%s %d opserver event_base_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	log_debug("aaaaaaaaaaaaaaaaaaaa\n");
	
	log_debug("bbb\n");
	sleep(1);
	oplog_exit(_op->log);
	if(event_base_loop(_op->base, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		printf ("%s %d opserver failed\n",__FILE__,__LINE__);
		goto exit;
	}
	return 0;
exit:
	opserver_exit(_op);
	return -1;
}
