#include "opmgr.h"
#include <stdlib.h>
#include <assert.h>
#include "interface/mgr.h"
#include "opmgr_bus_cb.h"
#include "system_info.h"
#include "sys/sysinfo.h"
#include "libubox/utils.h"
#include <string.h>

struct opmgr *opmgr_self = NULL;
struct opmgr *get_h_opmgr(void)
{
	assert(opmgr_self);

	return opmgr_self;
}


struct opmgr *opmgr_init(void)
{
	struct opmgr *mgr = NULL;
	char buf[1024] = {};
	int i = 0;
	struct proc_cpu_info * cpu_p;
	
	mgr = calloc(1, sizeof(struct opmgr));
	if (!mgr)
		goto out;

	mgr->t_cpu.tv_sec = 1;

	mgr->cpu_num = get_nprocs();
	if (mgr->cpu_num <= 0)
		goto out;

	mgr->cpu_usage = calloc(1,  sizeof(struct cpu_info)+mgr->cpu_num * sizeof(struct cpu_info));
	if (!mgr->cpu_usage)
		goto out;

	mgr->proc_cpu[0] = calloc(1, sizeof(struct proc_cpu_info)+mgr->cpu_num * sizeof(struct proc_cpu_info));
	if (!mgr->proc_cpu[0])
		goto out;

	mgr->proc_cpu[1] = calloc(1, sizeof(struct proc_cpu_info)+mgr->cpu_num * sizeof(struct proc_cpu_info));
	if (!mgr->proc_cpu[1])
		goto out;

	
	mgr->cpu_file = popen("cat /proc/stat", "r");
	if (!mgr->cpu_file)
		goto out;

	pthread_rwlock_init(&mgr->rwlock, NULL);

	cpu_p = mgr->proc_cpu[0];
	mgr->proc_cpu_use = 0;
	
	for (i = 0; i <= mgr->cpu_num; i++) {/*line is cpu num add a indent line*/
		fgets(buf, sizeof(buf), mgr->cpu_file);
		sscanf(buf,"%s %u %u %u %u %u %u %u %u %u", cpu_p[i].cpu_name,
			&cpu_p[i].user, &cpu_p[i].nice, &cpu_p[i].system,
			&cpu_p[i].idle,&cpu_p[i].iowait,&cpu_p[i].irq,
			&cpu_p[i].softirq,&cpu_p[i].steal, &cpu_p[i].guest);
	}
	
	pclose(mgr->cpu_file);
	opmgr_self = mgr;
	return mgr;

out:
	opmgr_exit(mgr);
	return NULL;
}

void opmgr_exit(struct opmgr *mgr)
{
	(void)mgr;

	return;
}

void opmgr_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)data;
	(void)size;
	(void)arg;
	int res_size = 0;
	
	struct opmgr *mgr = get_h_opmgr();
	
	if (to_sub_id >= mgr_max)
		return;

	if((res_size = handle_mgr_sub_id(from_module, to_sub_id, mgr->bus_buf, sizeof(mgr->bus_buf))) < 0)
		return;

	bus_send(h, to_sub_id, from_module, from_sub_id, mgr->bus_buf, res_size);

	return;
}
void opmgr_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}


void opmgr_cpu_calc_timer(int s, short what, void *arg)
{

	(void)s;
	(void)what;
	(void)arg;
	char buf[1024] = {};
	int i = 0;
	int cpu_use = 0;
	struct opmgr *mgr = NULL;
	float total = 0;	

	struct proc_cpu_info *cpu_p = NULL;
	struct proc_cpu_info *cpu_pre = NULL;
	
	mgr = get_h_opmgr();


	cpu_use = !mgr->proc_cpu_use;

	cpu_pre = mgr->proc_cpu[mgr->proc_cpu_use];

	cpu_p = mgr->proc_cpu[cpu_use];
	mgr->cpu_file = popen("cat /proc/stat", "r");
	if (!mgr->cpu_file)
		return;
	for (i = 0; i <= mgr->cpu_num; i++) {/*line is cpu num add a indent line*/
		fgets(buf, sizeof(buf), mgr->cpu_file);
		sscanf(buf,"%s %u %u %u %u %u %u %u %u %u", cpu_p[i].cpu_name,
			&cpu_p[i].user, &cpu_p[i].nice, &cpu_p[i].system,
			&cpu_p[i].idle,&cpu_p[i].iowait,&cpu_p[i].irq,
			&cpu_p[i].softirq,&cpu_p[i].steal, &cpu_p[i].guest);
			total = ((cpu_p[i].user+cpu_p[i].nice+cpu_p[i].system+cpu_p[i].idle+cpu_p[i].iowait+cpu_p[i].irq+cpu_p[i].softirq+cpu_p[i].steal+cpu_p[i].guest) - 
					(cpu_pre[i].user+cpu_pre[i].nice+cpu_pre[i].system+cpu_pre[i].idle+cpu_pre[i].iowait+cpu_pre[i].irq+cpu_pre[i].softirq+cpu_pre[i].steal+cpu_pre[i].guest))*1.0;

			pthread_rwlock_wrlock(&mgr->rwlock);
			strlcpy(mgr->cpu_usage[i].cpu_name, cpu_p[i].cpu_name, sizeof(mgr->cpu_usage[i].cpu_name));
			mgr->cpu_usage[i].user = (cpu_p[i].user - cpu_pre[i].user)/total *100.0;
			mgr->cpu_usage[i].nice = (cpu_p[i].nice - cpu_pre[i].nice)/total *100.0;
			mgr->cpu_usage[i].system = (cpu_p[i].system - cpu_pre[i].system)/total *100.0;
			mgr->cpu_usage[i].iowait = (cpu_p[i].iowait - cpu_pre[i].iowait)/total *100.0;
			mgr->cpu_usage[i].irq = (cpu_p[i].irq - cpu_pre[i].irq)/total *100.0;
			mgr->cpu_usage[i].softirq = (cpu_p[i].softirq - cpu_pre[i].softirq)/total *100.0;
			mgr->cpu_usage[i].steal = (cpu_p[i].steal - cpu_pre[i].steal)/total *100.0;
			mgr->cpu_usage[i].guest = (cpu_p[i].guest - cpu_pre[i].guest)/total *100.0;
			mgr->cpu_usage[i].cpu_use = (total - (cpu_p[i].idle - cpu_pre[i].idle))/total *100.0;
			pthread_rwlock_unlock(&mgr->rwlock);
	}

	mgr->proc_cpu_use = cpu_use;

	pclose(mgr->cpu_file);

	return;
}

