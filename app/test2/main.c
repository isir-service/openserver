#include "interface/module.h"
#include "interface/log.h"
#include "interface/bus.h"
#include <stdlib.h>
#include <stdio.h>
#include "event.h"

void test_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)h;
	(void)arg;
	printf("from module:%u, from sub_id:%u,to_sub_id:%u,data:%s, size = %u\n", from_module, from_sub_id , to_sub_id, (char*)data, size);
	bus_send(h, 0, from_module, from_sub_id, "good monring", 12);
	return;
}

void test_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	struct event_base * base = event_base_new();
	
	void *log_h = log_init(MODULE_OPCLI);
	(void)log_h;

	void *bus_h = bus_connect(MODULE_OPBUS, test_bus_cb, test_bus_disconnect, NULL);
	(void)bus_h;
	event_base_dispatch(base);
	return 0;
}
