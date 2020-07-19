#ifndef __CELL_MODULE_H__
#define __CELL_MODULE_H__
#include "cell_pub.h"
void cell_module_init(void);


void *module_thread_job(void *arg);

void module_call_timer(int s, short what, void *arg);


#endif
