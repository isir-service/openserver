#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "opbox/usock.h"
#include "base/oplog.h"
#include "base/opbus.h"
#include "base/opcli.h"

#include "event.h"
#include <signal.h>
struct _opserver_struct_ {
	void *log;
	void *bus;
	void *cli;
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

	oplog_exit(_op->log);
	return;
}

int cmd_cb1(int argc, const char **argv, struct cmd_element *ele, struct _vty * vty)
{
	int i = 0;
	opcli_out(vty, "cmd_cb1 argc:%d\r\n", argc);
	for (i =0; i< argc;i++) {
		opcli_out(vty, "argv:%s\r\n", argv[i]);
	}

	return 0;
}

int cmd_cb2(int argc, const char **argv, struct cmd_element *ele, struct _vty * vty)
{
	int i = 0;
	opcli_out(vty, "cmd_cb2 argc:%d\r\n", argc);
	for (i =0; i< argc;i++) {
		opcli_out(vty, "argv:%s\r\n", argv[i]);
	}

	
	return 0;
}

int cmd_cb3(int argc, const char **argv, struct cmd_element *ele, struct _vty * vty)
{
	int i = 0;
	opcli_out(vty, "cmd_cb3 argc:%d\r\n", argc);
	for (i =0; i< argc;i++) {
		opcli_out(vty, "argv:%s\r\n", argv[i]);
	}

	
	return 0;
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

	log_error("test log\n");
	log_warn("test log\n");
	log_info("test log\n");
	log_debug("test log\n");

	opcli_install_cmd(node_view, "helloworld b", "a\nb\n", cmd_cb1);
	
	opcli_install_cmd(node_view, "helloworld c", "c\nd\n", cmd_cb2);

	opcli_install_cmd(node_view, "(a|b) <1-100>", "e\nf\n", cmd_cb3);
	
	log_debug("install test element over\n");

	_op->base = event_base_new();
	if (!_op->base) {
		printf ("%s %d opserver event_base_new faild\n",__FILE__,__LINE__);
		goto exit;
	}
	
	self = _op;

	if(event_base_loop(_op->base, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		printf ("%s %d opserver failed\n",__FILE__,__LINE__);
		goto exit;
	}
	return 0;
exit:
	opserver_exit(_op);
	return -1;
}
