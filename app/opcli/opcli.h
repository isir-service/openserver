#ifndef _OPCLI_H__
#define _OP_CLI_H__

#include <cli/thread.h>
#include <event.h>
#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include <stdlib.h>
#include <sys/types.h>
#include "interface/bus.h"
#include <pthread.h>

struct opcli {
	struct thread_master * master;
	void *bus;
	void *log;
};

void *get_opcli_bus(void);


struct opcli *opcli_init(void);

void opcli_exit(struct opcli *cli);

void opcli_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg);
void opcli_bus_disconnect(void *h,void *arg);

#endif
