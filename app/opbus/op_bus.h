#ifndef __OP_BUS_H__
#define __OP_BUS_H__
#include "event.h"

#define MAX_JOB_THREAT_NUM 10
struct _op_bus {
	void *log;
	struct event_base *ebase_sche;
	struct event_base *ebase_job[MAX_JOB_THREAT_NUM];

};

struct _op_bus * opbus_init(void);

void opbus_exit(struct _op_bus *bus);


#endif
