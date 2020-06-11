#include "op_bus.h"

#include <assert.h>
#include <stdlib.h>
#include "interface/log.h"

static struct _op_bus *self;

#define opbus_log_error(fmt...) log_err(self->log,fmt)
#define opbus_log_warn(fmt...) log_warn(self->log,fmt)
#define opbus_log_info(fmt...) log_info(self->log,fmt)
#define opbus_log_debug(fmt...) log_debug(self->log,fmt)


struct _op_bus *get_h_opbus(void)
{
	assert(self);

	return self;
}

void set_h_opbus(struct _op_bus *bus)
{
	self = bus;
	return;
}

struct _op_bus * opbus_init(void)
{
	struct _op_bus *bus = calloc(1, sizeof(struct _op_bus));
	set_h_opbus(bus);
	return bus;
}

void opbus_exit(struct _op_bus *bus)
{
	if (!bus)
		return;

	return;
}


