#ifndef __LOG_H__
#define __LOG_H__

#include "module.h"

#define LOG_BUF_MAX 40000

#define LOG_CONF_PATH "/home/isir/developer/openserver/app/oplog/oplog_conf.xml"

void *log_init(unsigned int module);

void log_exit(void *h);

void log_err(void *h, const char *fmt, ...);
void log_warn(void *h, const char *fmt, ...);
void log_info(void *h, const char *fmt, ...);
void log_debug(void *h, const char *fmt, ...);


#endif
