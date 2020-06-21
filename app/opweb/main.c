#include <stdio.h>
#include "opweb.h"
#include "interface/module.h"
#include "interface/log.h"
#include "libubox/utils.h"
#include "interface/bus.h"
#include "libubox/usock.h"
#include <event.h>

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	int i = 0;
	struct opweb *web = NULL; 
	web = opweb_init();
	if (!web)
		goto out;
	web->base = event_base_new();
	if (!web->base)
		goto out;

	web->log = log_init(MODULE_OPWEB);
	if (!web->log)
		goto out;

	web->bus = bus_connect(MODULE_OPWEB, opweb_bus_cb, opweb_bus_disconnect, web);
	if (!web->bus)
		goto out;

	web->http_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(60000));
	if (web->http_fd < 0)
		goto out;

	web->https_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(60001));
	if (web->https_fd < 0)
		goto out;

	for (i = 0; i < MAX_OPWEB_THREAT_NUM; i++) {
		web->thread[i].base = event_base_new();
		web->thread[i].index = i;
		if (!web->thread[i].base)
			goto out;

		if (pthread_create(&web->thread[i].thread_id, NULL, opweb_thread_job, web->thread[i].base))
			goto out;
	}

	web->ehttp = event_new(web->base, web->http_fd, EV_PERSIST|EV_READ, opweb_http_accept, web);
	if (event_add(web->ehttp, NULL))
		goto out;

	event_base_dispatch(web->base);

	return 0;
out:
	opweb_exit(web);
	return 0;
}
