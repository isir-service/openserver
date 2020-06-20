#ifndef __BUS__
#define __BUS__
#include <pthread.h>
typedef void (*bus_read_cb)(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg);
typedef void (*bus_disconnect)(void *h,void *arg);
void *bus_connect(unsigned int module, bus_read_cb cb, bus_disconnect disconnect, void *arg);
void bus_exit(void *h);
int bus_send(void *h,unsigned int from_sub_id,  unsigned int to_module, unsigned int to_sub_id, void *data, unsigned int size);
int bus_send_sync(void *h,unsigned int from_sub_id,  unsigned int to_module, unsigned int to_sub_id, void *data, unsigned int size, void*res, unsigned int res_size,unsigned int *res_actual, unsigned int ms);
#endif
