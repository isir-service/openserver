#include <stdio.h>
#include "op_bus.h"
#include "interface/module.h"
#include "interface/log.h"
#include "pthread.h"
#include "libubox/usock.h"
#include <unistd.h>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

#define BACKTRACE_SIZE   16
 
void dump_trace(int signo)
{
	(void)signo;
	int j, nptrs;
	void *buffer[BACKTRACE_SIZE];
	char **strings;
	
	nptrs = backtrace(buffer, BACKTRACE_SIZE);
	
	printf("backtrace() returned %d addresses\n", nptrs);
 
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		return;
	}

	for (j = 0; j < nptrs; j++)
		printf("  [%02d] %s\n", j, strings[j]);

	free(strings);

}


int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	signal(SIGSEGV, dump_trace);
	int i = 0;
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

		bus->thread[i].trigger = event_new(bus->thread[i].ebase_job, -1, EV_PERSIST, opbus_trigger, NULL);
		if (!bus->thread[i].trigger)
			goto out;

		if (event_add(bus->thread[i].trigger, NULL) < 0)
			goto out;
		
		if (pthread_create(&bus->thread[i].thread_id, NULL, opbus_thread_job, bus->thread[i].ebase_job))
			goto out;
	}


	event_base_dispatch(bus->ebase_sche);
out:
	opbus_exit(bus);
	return 0;
}
