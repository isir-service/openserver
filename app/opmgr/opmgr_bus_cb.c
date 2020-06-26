#include "opmgr_bus_cb.h"
#include "system_info.h"
#include <stdio.h>
#include <sys/sysinfo.h>
#include "interface/mgr.h"
#include "interface/module.h"

typedef int (*opmgr_bus_cb)(unsigned int from_module, unsigned char *encap_buf, unsigned int size);


struct sub_id_opmgr_map {
	opmgr_bus_cb cb;
};

struct sub_id_opmgr_map opmgr_map[mgr_max] = {
	
	[mgr_meminfo] = {.cb = opmgr_get_meminfo},

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

