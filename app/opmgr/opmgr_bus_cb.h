#ifndef __OPMGR_BUS_CB_H__
#define __OPMGR_BUS_CB_H__

int handle_mgr_sub_id(unsigned int from_module, unsigned int sub_id, unsigned char *encap_buf, unsigned int size);

int opmgr_get_meminfo(unsigned int from_module, unsigned char *encap_buf, unsigned int size);

#endif
