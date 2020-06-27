#include "opmgr_bus_cb.h"
#include "system_info.h"
#include <stdio.h>
#include <sys/sysinfo.h>
#include "interface/mgr.h"
#include "interface/module.h"
#include "opmgr.h"

typedef int (*opmgr_bus_handle_cb)(unsigned int from_module, unsigned char *encap_buf, unsigned int size);


struct sub_id_opmgr_map {
	opmgr_bus_handle_cb cb;
};

struct sub_id_opmgr_map opmgr_map[mgr_max] = {
	
	[mgr_meminfo] = {.cb = opmgr_get_meminfo},
	[mgr_cpu_usage] = {.cb = opmgr_get_cpu_usage},

};


int handle_mgr_sub_id(unsigned int from_module, unsigned int sub_id, unsigned char *encap_buf, unsigned int size)
{
	if (sub_id >= mgr_max || !encap_buf || !size || from_module >= MODULE_MAX)
		goto out;

	return opmgr_map[sub_id].cb(from_module, encap_buf, size);

out:
	return -1;
}

int opmgr_get_meminfo(unsigned int from_module, unsigned char *encap_buf, unsigned int size)
{
	struct sysinfo info;
	int ret = 0;
	int index = 0;
	char *str = (char*)encap_buf;
	
	if (!str || !size)
		goto out;

	if (sysinfo(&info) < 0)
		goto out;

	if (from_module == MODULE_OPCLI) {
		index = 0;
		ret = snprintf (str+index,size-index,"total :%luMB\r\n", info.totalram/1024/1024);
		if (ret < 0)
			goto out;

		index += ret;
		ret = snprintf (str+index,size-index,"avalid:%luMB", info.freeram/1024/1024);
		if (ret < 0)
			goto out;
		
		index += ret;

		return index;
	}
	
out:
	return -1;
}

int opmgr_get_cpu_usage(unsigned int from_module, unsigned char *encap_buf, unsigned int size)
{

	char *str = (char*)encap_buf;

	struct opmgr *mgr = get_h_opmgr();
	
	int index = 0;
	int ret = 0;
	int i = 0;
	char *str_form = "\r\n";

	if (!str || !size)
		return -1;

	pthread_rwlock_rdlock(&mgr->rwlock);
	if (from_module == MODULE_OPCLI) {
		index = 0;
		ret = snprintf(str+index,size-index,"name      %%user       %%nice       %%system      %%iowait      %%steal      %%usage\r\n");
		if (ret < 0)
			goto out;

		index += ret;

		for (i = 0; i <= mgr->cpu_num;i++) {
			if (i == mgr->cpu_num)
				str_form ="";
			else
				str_form = "\r\n";

			ret = snprintf (str+index,size-index,"%-6s    %-6.2f      %-6.2f      %-7.2f      %-7.2f      %-6.2f      %-6.2f%s",
				mgr->cpu_usage[i].cpu_name, mgr->cpu_usage[i].user, mgr->cpu_usage[i].nice,mgr->cpu_usage[i].system,
				mgr->cpu_usage[i].iowait,mgr->cpu_usage[i].steal,mgr->cpu_usage[i].cpu_use,str_form);

			if (ret < 0)
				goto out;
			
			index += ret;
		}
		
		pthread_rwlock_unlock(&mgr->rwlock);
		return index;
	}
out:
	pthread_rwlock_unlock(&mgr->rwlock);
	return -1;
}


