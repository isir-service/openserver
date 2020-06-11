#include <stdio.h>
#include "op_bus.h"
#include "interface/module.h"
#include "interface/log.h"
int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	struct _op_bus *bus = opbus_init();
	if (!bus) {
		printf ("opbus_init");
		goto out;
	}

	bus->log = log_init(MODULE_OPBUS);
	if (!bus->log)
		goto out;

out:
	opbus_exit(bus);
	return 0;
}
