#ifndef _OPRPC_H__
#define _OPRPC_H__

/********************tipc*******************************/
#define TIPC_MAX_ELEMENT 1024
#define TIPC_REQ_SIZE 4096
#define TIPC_RESPONSE_SIZE 8192

enum {
	rpc_tipc_module_none = 9999,
	rpc_tipc_module_opserver,
	rpc_tipc_module_max,
};

enum {
	tipc_opserver_none,
	tipc_opserver_cup_usage,
	tipc_opserver_show_mem_poll,
	tipc_opserver_send_quotes,
	tipc_opserver_max,
};

typedef int (*rpc_tipc_cb)(unsigned char *req, int req_size, unsigned char *response, int res_size);

int op_tipc_init(unsigned int module);
int op_tipc_register(unsigned int type, rpc_tipc_cb cb);
int op_tipc_send(unsigned int module, unsigned int type, unsigned char *req, unsigned int size);
int op_tipc_send_ex(unsigned int module, unsigned int type, unsigned char *req, unsigned int size, unsigned char *response, int response_size);

/*********************tipc end****************************************/

#endif
