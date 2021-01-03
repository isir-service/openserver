#ifndef _U_9300_H__
#define _U_9300_H__

#include "op4g_handle.h"

int u9300_init(int fd);

void u9300_exit(void);

int u9300_handle(int fd, unsigned int event_type, struct _4g_handle *handle);
#endif
