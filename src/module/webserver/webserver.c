#include <stdio.h>
#include <stdlib.h>

#include "webserver.h"
#include "nginx.h"

void *webserver_init(void)
{
	int ac;
	static char **av;
	static char buf[6][40];
	av=(char **)calloc(5, sizeof(char*));
	snprintf(buf[0],40,"nginx");
	snprintf(buf[1],40,"-p");
	snprintf(buf[2],40,"/home/opserver/etc/webserver/");
	snprintf(buf[3],40,"-c");
	snprintf(buf[4],40,"webserver.conf");
	ac = 5;
	av[0] = buf[0];
	av[1] = buf[1];
	av[2] = buf[2];
	av[3] = buf[3];
	av[4] = buf[4];

	printf("webserver_init\n");
	nginx_main(ac, av);
	printf("webserver_init over\n");

	return NULL;
}

void webserver_exit(void *web)
{
	return;
}
