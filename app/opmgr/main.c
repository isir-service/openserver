#include "interface/bus.h"
#include "event.h"
#include "interface/log.h"
#include "opmgr.h"
#include "libubox/utils.h"

int main (int argc, char **argv)
{
	(void)argc;
	(void)argv;
	
	signal_action();

	struct opmgr *mgr = opmgr_init();
	if (!mgr)
		goto out;

	mgr->log = log_init(MODULE_OPMGR);
	if (!mgr->log)
		goto out;

	log_debug(mgr->log, "opmgr init\n");
	mgr->bus = bus_connect(MODULE_OPMGR, opmgr_bus_cb, opmgr_bus_disconnect, NULL);
	if (!mgr->bus)
		goto out;

	mgr->base = event_base_new();
	if (!mgr->base)
		goto out;

	mgr->et_cpu = event_new(mgr->base, -1, EV_PERSIST, opmgr_cpu_calc_timer, mgr);
	if (!mgr->et_cpu)
		goto out;

	if (event_add(mgr->et_cpu, &mgr->t_cpu) < 0)
		goto out;

	event_base_loop(mgr->base,EVLOOP_NO_EXIT_ON_EMPTY);

	return 0;

out:
	opmgr_exit(mgr);
	return 0;
}
