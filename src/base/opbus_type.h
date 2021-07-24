#ifndef _OPBUS_TYPE_H__
#define _OPBUS_TYPE_H__

struct _bus_req_head{
	unsigned int type;
};

struct _bus_response_head{
	unsigned int type;
};

#define _BUS_BUF_RESPONSE_SIZE 8192
#define _BUS_BUF_REQ_SIZE 4096
#define _BUS_WAIT_MS 7000
#define BUS_SERVER "opbus:bus_ip"
#define BUS_PORT "opbus:bus_port"

enum {
	opbus_test = 0,
	/*****************opmgr************************/
	opbus_opmgr_get_cpu_usage,
	opbus_opmgr_show_mem_poll,

	/*****************op4g************************/
	opbus_op4g_send_quotes,
	
	/*****************spider************************/
	opbus_spider_check_stock,
	
	opbus_max,
};

#endif

