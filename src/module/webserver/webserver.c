#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "webserver.h"
#include "nginx.h"
#include "base/oplog.h"
#include "config.h"
#include "opbox/utils.h"

struct _webserver_struct {
	int nginx_pid;
};

static struct _webserver_struct *self = NULL;
void nginx_main_start(char *opserver_conf_path)
{
	int argc = 1;
	char argv_buf[64];
	char **argv = calloc(1, sizeof(char*));
	if (!argv) {
		log_error("calloc failed[%d]\n", errno);
		return;
	}
	strlcpy(argv_buf, "nginx", sizeof(argv_buf));
	argv[0] = argv_buf;

	nginx_main(argc, argv, OPSERVER_CONF);
}

void *webserver_init(void)
{
	struct _webserver_struct *web = NULL;
	int pid = 0;

	web = calloc(1, sizeof(*web));
	if (!web) {
		log_error("calloc failed[%d]\n", errno);
		goto exit;
	}

	self = web;

	pid = fork();
	if (pid < 0) {
		log_error("fork failed[%d]\n", errno);
		goto exit;
	}

	if (!pid)
		nginx_main_start(OPSERVER_CONF);

	web->nginx_pid = pid;
	return web;

exit:
	webserver_exit(web);
	return NULL;
}

void webserver_exit(void *web)
{
	return;
}
