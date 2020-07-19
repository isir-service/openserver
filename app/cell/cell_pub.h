#ifndef __CELL_PUB_H__
#define __CELL_PUB_H__

#include "event.h"
#include <pthread.h>
#include "interface/log.h"

enum {
	LTE_MODULE_U9300C = 1,

	LTE_MODULE_MAX,

};


struct sim_info {
	int carrier;

};

struct module_detail {
	char module_name[64];

};

struct lte_module_info;

typedef int (*lte_module_init)(struct lte_module_info *);
typedef int (*module_response)(unsigned char *, int , struct lte_module_info *);

struct module_at_map {
	char *at_cmd;
	module_response cb;
};

struct lte_module_buf {
	unsigned char buf[496];
	int buf_size;
	int index;
};

struct lte_module_info {
	int vendor_id;
	int product_id;
	char *at_dev;
	struct sim_info sim;
	struct module_detail module;
	int fd;
	lte_module_init init;
	struct event_base *base;
	struct event *read;
	pthread_t thread_id;
	struct event *call_init;
	struct timeval call_t;
	struct module_at_map *current_map;
	struct lte_module_buf read_buf;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int use;
};

struct lte_module_info lte_module[LTE_MODULE_MAX];

struct op_cell {
	void *log;
	void *bus;
	struct event_base *base;
	struct lte_module_info *module;
};


void lte_release_module(struct lte_module_info *module);
struct op_cell *lte_self;

struct op_cell *get_cell_self(void);

#define cell_log_error(fmt...) log_err(lte_self->log,fmt)
#define cell_log_warn(fmt...) log_warn(lte_self->log,fmt)
#define cell_log_info(fmt...) log_info(lte_self->log,fmt)
#define cell_log_debug(fmt...) log_debug(lte_self->log,fmt)



#endif
