#include "interface/bus.h"
#include "event.h"
#include "interface/log.h"
#include "opmgr.h"

int main (int argc, char **argv)
{
	(void)argc;
	(void)argv;
	struct opmgr *mgr = opmgr_init();
	if (!mgr)
		goto out;

	mgr->log = log_init(MODULE_OPMGR);
	if (!mgr->log)
		goto out;

	mgr->bus = bus_connect(MODULE_OPMGR, opmgr_bus_cb, opmgr_bus_disconnect, NULL);
	if (!mgr->bus)
		goto out;

	mgr->base = event_base_new();
	if (!mgr->base)
		goto out;

	event_base_loop(mgr->base,EVLOOP_NO_EXIT_ON_EMPTY);

	return 0;

out:
	opmgr_exit(mgr);
	return 0;
}
