#ifndef _OP_LOG_H__
#define _OP_LOG_H__


#define LOG_PATH "./oplog.conf"
#define ZLOG_PATH "./zlog.conf"

typedef enum {

	_oplog_fatal = 1,
	_oplog_error,
	_oplog_warn ,
	_oplog_info ,
	_oplog_debug,
} oplog_level;


char get_log_level(const char *buf);
unsigned int get_log_max_payload_size(void);
const char *get_log_payload(const char *buf);


void *log_init(const char *module_name);

void log_exit(void *handle);

void log_send(void *handle, int level,const char *format, ...);

void log_fatal(void *handle, const char *format, ...);
void log_error(void *handle ,const char *format, ...);
void log_warn(void *handle, const char *format, ...);
void log_info(void *handle, const char *format, ...);
void log_debug(void *handle, const char *format, ...);

#endif