#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include "opmgr.h"
#include "opbox/usock.h"
#include "base/oplog.h"
#include "base/opbus.h"
#include "base/opcli.h"
#include "opbox/utils.h"
#include "event.h"
#include "base/opbus_type.h"
#include "opmgr_cmd.h"
#include "opmgr_bus.h"

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

struct _mgr_thread_ 
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
};

struct _mgr_timer {
	struct event *cpu_usage;
};

struct _opmgr_struct {
	struct event_base *base;
	struct _mgr_thread_ thread;
	struct _mgr_timer timer;
	struct cpu_info _cpu_info;
};

static struct _opmgr_struct  *self;

static void *opmgr_routine (void *arg)
{
	if(event_base_loop(arg, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		log_error("opmgr_routine failed\n");
		pthread_detach(pthread_self());
		pthread_exit(NULL);
		goto exit;
	}

	log_debug ("opmgr_routine exit\n");
exit:
	return NULL;
}

static void _get_cpu_info(struct proc_cpu_info *cpu_info, int cpu_num)
{
	FILE *cpu_file;
	int i = 0;
	char buf[128] = {};
	int ret = 0;

	cpu_file = popen("cat /proc/stat", "r");
	if (!cpu_file) {
		log_warn("popen cpu file failed\n");
		goto exit;
	}
	
	for (i = 0; i <= cpu_num; i++) {
		if (!fgets(buf, sizeof(buf), cpu_file)) {
			log_warn("fgets cpu file failed\n");
			goto exit;
		}

		ret = sscanf(buf,"%s %u %u %u %u %u %u %u %u %u", cpu_info[i].cpu_name,
				&cpu_info[i].user, &cpu_info[i].nice, &cpu_info[i].system,
				&cpu_info[i].idle,&cpu_info[i].iowait,&cpu_info[i].irq,
				&cpu_info[i].softirq,&cpu_info[i].steal, &cpu_info[i].guest);
		if (ret != 10) {
			log_warn("sscanf num failed\n");
			goto exit;
		}
	}
	
exit:
	if (cpu_file)
		fclose(cpu_file);

	return;
}

static int _get_cpu_info_by_file(struct cpu_info_usage *cpu_usage, int cpu_num, int how_ms)
{
	int buf_size = 0;
	int i = 0;
	float total = 0.0;

	buf_size = sizeof(struct proc_cpu_info) * cpu_num + 1;

	if (2*buf_size > 4096) {
		log_warn("buf_size too long [%d]\n", buf_size*2);
		return -1;
	}

	struct proc_cpu_info cpu_pre[cpu_num+1];
	struct proc_cpu_info cpu_after[cpu_num+1];

	_get_cpu_info(cpu_pre, cpu_num);
	usleep(how_ms*1000);
	_get_cpu_info(cpu_after, cpu_num);

	for (i = 0; i <= cpu_num; i++) {
		total = ((cpu_after[i].user+cpu_after[i].nice+cpu_after[i].system+cpu_after[i].idle+cpu_after[i].iowait+cpu_after[i].irq+
				cpu_after[i].softirq+cpu_after[i].steal+cpu_after[i].guest) - 
				(cpu_pre[i].user+cpu_pre[i].nice+cpu_pre[i].system+cpu_pre[i].idle+cpu_pre[i].iowait+cpu_pre[i].irq+cpu_pre[i].softirq+
				cpu_pre[i].steal+cpu_pre[i].guest))*1.0;
		strlcpy(cpu_usage[i].cpu_name, cpu_pre[i].cpu_name, sizeof(cpu_usage[i].cpu_name));
		cpu_usage[i].user = (cpu_after[i].user - cpu_pre[i].user)/total *100.0;
		cpu_usage[i].nice = (cpu_after[i].nice - cpu_pre[i].nice)/total *100.0;
		cpu_usage[i].system = (cpu_after[i].system - cpu_pre[i].system)/total *100.0;
		cpu_usage[i].iowait = (cpu_after[i].iowait - cpu_pre[i].iowait)/total *100.0;
		cpu_usage[i].irq = (cpu_after[i].irq - cpu_pre[i].irq)/total *100.0;
		cpu_usage[i].softirq = (cpu_after[i].softirq - cpu_pre[i].softirq)/total *100.0;
		cpu_usage[i].steal = (cpu_after[i].steal - cpu_pre[i].steal)/total *100.0;
		cpu_usage[i].guest = (cpu_after[i].guest - cpu_pre[i].guest)/total *100.0;
		cpu_usage[i].cpu_use = (total - (cpu_after[i].idle - cpu_pre[i].idle))/total *100.0;
	}

	return 0;
}

static void _mgr_cpu_usage_period(evutil_socket_t fd , short what, void *arg)
{
	int cpu_num = 0;
	cpu_num = self->_cpu_info.cpu_num;

	_get_cpu_info_by_file(arg ,cpu_num, 500);
	

	return;
}

int format_cpu_usage1(unsigned int type,unsigned char *response,int res_size)
{
	struct cpu_info *cpu_usage = NULL;
	int i = 0;
	cpu_usage = (struct cpu_info *)response;
	printf("name      %%user       %%nice       %%system      %%iowait      %%steal      %%usage\n");

	for(i = 0; i <= cpu_usage->cpu_num;i++) {
		printf("%-6s    %-6.2f      %-6.2f      %-7.2f      %-7.2f      %-6.2f      %-6.2f\n",
				cpu_usage->usage[i].cpu_name, cpu_usage->usage[i].user,cpu_usage->usage[i].nice, cpu_usage->usage[i].system,
				cpu_usage->usage[i].iowait, cpu_usage->usage[i].steal, cpu_usage->usage[i].cpu_use);
	}

	return 0;

}


int bus_get_cpu_usage(unsigned char *req, int req_size, unsigned char *response, int res_size)
{
	int src_size;
	int ret = 0;
	src_size = sizeof(struct cpu_info);
	if (src_size > res_size)
		log_warn("cpu usage, message truncate[src_size=%d, res_size=%d]\n", src_size, res_size);
	
	ret = memlcpy(response,res_size, &self->_cpu_info, src_size);
	return ret;
}

static void _mgr_bus_register(void)
{
	opbus_register(opbus_opmgr_get_cpu_usage, bus_get_cpu_usage);
	return;
}

void *opmgr_init(void)
{
	struct _opmgr_struct  *mgr = NULL;
	struct timeval tv;

	mgr = calloc(1, sizeof(*mgr));
	if (!mgr) {
		log_error(" calloc failed[%d]\n",errno);
		goto exit;
	}

	mgr->base = event_base_new();
	if (!mgr->base) {
		log_error ("event_base_new failed\n");
		goto exit;
	}
	
	self = mgr;

	mgr->_cpu_info.cpu_num = get_nprocs();

	log_info("cpu num:%d\n", mgr->_cpu_info.cpu_num);

	mgr->_cpu_info.cpu_num = mgr->_cpu_info.cpu_num > MAX_CPU_CORE?MAX_CPU_CORE:mgr->_cpu_info.cpu_num;

	_get_cpu_info_by_file(mgr->_cpu_info.usage , mgr->_cpu_info.cpu_num, 500);

	_mgr_bus_register();
	opmgr_cmd_init();

	mgr->timer.cpu_usage = event_new(mgr->base, -1, EV_PERSIST, _mgr_cpu_usage_period, mgr->_cpu_info.usage);
	if (!mgr->timer.cpu_usage) {
		log_error ("opmgr event_new failed\n");
		goto exit;
	}

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	if(event_add(mgr->timer.cpu_usage, &tv) < 0) {
		log_error ("opmgr event_add faild\n");
		goto exit;
	}

	if(pthread_attr_init(&mgr->thread.thread_attr)) {
		log_error ("opmgr pthread_attr_init faild\n");
		goto exit;
	}

	if(pthread_create(&mgr->thread.thread_id, &mgr->thread.thread_attr, opmgr_routine, mgr->base)) {
		log_error ("opmgr pthread_create faild\n");
		goto exit;
	}

	log_debug ("opmgr thread_id[%x]\n", (unsigned int)mgr->thread.thread_id);

	return mgr;
exit:
	opmgr_exit(mgr);
	return NULL;
}

void opmgr_exit(void *mgr)
{
	return;
}

