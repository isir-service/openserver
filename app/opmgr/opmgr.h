#ifndef __OPMGR_H__
#define __OPMGR_H__
#include "event.h"
#include "interface/bus.h"

struct opmgr {
	struct event_base *base;
	void *bus;
	void *log;
	unsigned char bus_buf[BUS_RCV_BUF_MAX];
};

struct opmgr *opmgr_init(void);

void opmgr_exit(struct opmgr *mgr);


void opmgr_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg);
void opmgr_bus_disconnect(void *h,void *arg);

#endif
