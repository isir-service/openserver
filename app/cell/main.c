#include <stdio.h>
#include "cell.h"
#include "interface/module.h"
#include "interface/bus.h"
#include "event.h"
#include "cell_module.h"
#include "interface/log.h"
#include "libubox/utils.h"

int main(int argc, char*argv[])
{
	(void)argc;
	(void)argv;

	signal_action();

	struct op_cell *cell = cell_init();
	cell->log = log_init(MODULE_CELL);
	if (!cell->log)
		goto out;

	cell->bus = bus_connect(MODULE_CELL, cell_bus_cb, cell_bus_disconnect, NULL);
	if (!cell->bus)
		goto out;

	cell->base = event_base_new();
	if (!cell->base)
		goto out;

	/* init module */
	cell_module_init();

	event_base_loop(cell->base,EVLOOP_NO_EXIT_ON_EMPTY);
out:
	cell_exit(cell);
	return 0;
}
