#ifndef __OPMGR_H__
#define __OPMGR_H__
#include "event.h"
#include "interface/bus.h"
#include <stdio.h>

struct cpu_info {
	char cpu_name[64];
	float user;
	float nice;
	float system;
	float idle;
	float iowait;
	float irq;
	float softirq;
	float steal;
	float guest;
	float cpu_use;
};


struct proc_cpu_info {
	char cpu_name[64];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
	unsigned int iowait;
	unsigned int irq;
	unsigned int softirq;
	unsigned int steal;
	unsigned int guest;
};

struct opmgr {
	pthread_rwlock_t rwlock;
	struct event_base *base;
	void *bus;
	void *log;
	unsigned char bus_buf[BUS_RCV_BUF_MAX];

	FILE *cpu_file;
	struct event *et_cpu;
	struct timeval t_cpu;
	struct proc_cpu_info *proc_cpu[2];
	int proc_cpu_use;
	struct cpu_info *cpu_usage;
	int cpu_num;
};

struct opmgr *opmgr_init(void);
struct opmgr *get_h_opmgr(void);

void opmgr_exit(struct opmgr *mgr);


void opmgr_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg);
void opmgr_bus_disconnect(void *h,void *arg);

void opmgr_cpu_calc_timer(int s, short what, void *arg);



#endif
