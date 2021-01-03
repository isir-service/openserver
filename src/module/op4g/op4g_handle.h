#ifndef _OP4G_HANDLE_H__
#define _OP4G_HANDLE_H__

#include "opbox/list.h"

enum {
	_4G_EVENT_NONE,
	_4G_EVENT_READ,
	_4G_EVENT_MORE,
	_4G_EVENT_NEXT,
};

enum {
	_4G_CMD_AT_TEST,
	_4G_CMD_VENDOR_NAME,
	_4G_CMD_MODULE_TYPE,
	_4G_CMD_MODULE_IMEI,
	_4G_CMD_MAX,
};

struct _4g_cmd {
	struct list_head list;
	char *at;
	int (*at_cmd_cb)(struct _4g_cmd *cmd, char *resp, int resp_size);
};

struct _4g_handle {
	unsigned char *req;
	unsigned int req_size;
	unsigned char *resp;
	unsigned int resp_size;
};

#endif
