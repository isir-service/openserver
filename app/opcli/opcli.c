#include "opcli.h"
#include <stdlib.h>

static struct opcli *self;

void*get_opcli_bus(void)
{
	return self->bus;
}

struct opcli *opcli_init(void)
{
	struct opcli *cli = calloc(1, sizeof(struct opcli));
	if (!cli)
		return NULL;
	
	self = cli;
	return cli;
}

void opcli_exit(struct opcli *cli)
{
	(void)cli;
	return;
}

void opcli_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)h;
	(void)from_module;
	(void)from_sub_id;
	(void)to_sub_id;
	(void)data;
	(void)size;
	(void)arg;

	return;

}

void opcli_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}

