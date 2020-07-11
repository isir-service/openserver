#include <stdio.h>
#include "op_bus.h"
#include "interface/module.h"
#include "interface/bus.h"

#include "interface/log.h"
#include "pthread.h"
#include "libubox/usock.h"
#include <unistd.h>
#include "libubox/utils.h"

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	int i = 0;
	
	signal_action();

	struct _op_bus *bus = opbus_init();
	if (!bus) {
		printf ("opbus_init");
		goto out;
	}

	bus->log = log_init(MODULE_OPBUS);
	if (!bus->log)
		goto out;

	bus->ebase_sche = event_base_new();
	if (!bus->ebase_sche)
		goto out;

	bus->fd = usock(USOCK_SERVER|USOCK_TCP, "127.0.0.1", usock_port(55556));
	if (bus->fd < 0)
		goto out;

	bus->sch_read = event_new(bus->ebase_sche, bus->fd, EV_PERSIST|EV_READ, opbus_accept, bus);
	if (!bus->sch_read)
		goto out;

	if (event_add(bus->sch_read, NULL) < 0)
		goto out;

	for (i = 0; i < MAX_JOB_THREAT_NUM; i++) {
		bus->thread[i].ebase_job = event_base_new();
		if (!bus->thread[i].ebase_job)
			goto out;

		if (!i) {
			bus->timer.timer = evtimer_new(bus->thread[i].ebase_job, opbus_timer, &bus->timer);
			if (!bus->timer.timer)
				goto out;
			
			bus->timer.t.tv_sec = 2;
			if (event_add(bus->timer.timer, &bus->timer.t) < 0)
				goto out;

		}

		
		if (pthread_create(&bus->thread[i].thread_id, NULL, opbus_thread_job, bus->thread[i].ebase_job))
			goto out;
	}


	event_base_dispatch(bus->ebase_sche);
out:
	opbus_exit(bus);
	return 0;
}
