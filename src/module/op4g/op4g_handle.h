#ifndef _OP4G_HANDLE_H__
#define _OP4G_HANDLE_H__

#include "opbox/list.h"
#define _4G_IFACE_REQ_NUM 4
#define _4G_MESSAGE_SIZE 1024
enum {
	_4G_EVENT_NONE,
	_4G_EVENT_READ,
	_4G_EVENT_MORE,
	_4G_EVENT_NEXT,
};

enum {
	_4G_EVENT_SEND_MESSAGE,

};

enum {
	_4G_CMD_AT_TEST,
	_4G_CMD_VENDOR_NAME,
	_4G_CMD_MODULE_TYPE,
	_4G_CMD_MODULE_IMEI,
	_4G_MESSAGE_SET_TEXT,
	_4G_MESSAGE_SET_PDU,
	_4G_MESSAGE_SET_ENCODE,
	_4G_MESSAGE_SET_TEXT_PARAM,
	_4G_MESSAGE_SEND,
	_4G_AT_TMP,
	_4G_CMD_MAX,
};

struct _4g_cmd {
	unsigned int cmd;
	struct list_head list;
	char *at;
	int at_size;
	int (*at_cmd_cb)(struct _4g_cmd *cmd, unsigned char *resp, int resp_size);
};

struct _4g_uart_handle {
	unsigned char *req;
	unsigned int req_size;
	unsigned char *resp;
	unsigned int resp_size;
};

struct _4g_iface_handle {
	char *req[_4G_IFACE_REQ_NUM];
	int req_size[_4G_IFACE_REQ_NUM];
};


#endif
