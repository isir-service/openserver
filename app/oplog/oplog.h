#ifndef __OPLOG__H
#define __OPLOG__H
#include "event.h"
#include <stdlib.h>
#include "interface/module.h"
#include "interface/log.h"

struct debug_module {
	int fd;
	char module_name[120];
	int valid;
};

struct log_conf{
	char root_path[240];
	char date_path[412];
	int port;
	struct debug_module debug[MODULE_MAX];
};

struct oplog{
	struct event_base *ebase;
	int udp_fd;
	struct event *read;
	struct event *tm;
	struct timeval timeout;
	struct log_conf conf;
	void *bus;
	void *log;
	char buf[LOG_BUF_MAX];
};


struct oplog *oplog_init(void);
void oplog_exit(struct oplog *h_log);

struct oplog *get_h_oplog(void);
void set_h_oplog(struct oplog *h_log);

int oplog_parse_conf(const char *file_name);

void op_log_apply_conf(void);

void oplog_read(int s, short what, void *arg);


void oplog_timer(int s, short what, void *arg);

void oplog_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg);
void oplog_bus_disconnect(void *h,void *arg);


#endif
