#ifndef __CELL_H__
#define __CELL_H__

#include "cell_pub.h"

struct op_cell *cell_init(void);

void cell_exit(struct op_cell *cell);
void cell_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg);
void cell_bus_disconnect(void *h,void *arg);


#endif
