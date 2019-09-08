#include "oplog.h"
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <libubox/utils.h>
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>
#include <stdlib.h>



struct log_handle {
	struct log_header header;
	int sock;
	void *memery_handle;
	pthread_mutex_t lock;
	char buf[HEADE_SIZE + PAYLOAD_SIZE];
	struct sockaddr_in dest;

};





void *log_init(const char *module_name)
{
	
	struct log_handle *handle = NULL;
	void *memery_handle = NULL;
	dictionary *dict = NULL;
	const char* pstr = NULL;
	unsigned int dest_ip;
	unsigned short dest_port;
	char port_tmp[20] = {};
	
	if (module_name == NULL) {
		return NULL;
	}
	
	memery_handle= op_memery_init();

	if (memery_handle == NULL) {
		return NULL;
	}

	handle = op_calloc (memery_handle,1, sizeof (struct log_handle *));
	if (handle == NULL) {
		op_memery_exit (memery_handle);
		return NULL;
	}

	pthread_mutex_init(&handle->lock, NULL);
	handle->memery_handle = memery_handle;


	dict = iniparser_load(LOG_PATH);
	if (dict == NULL) {
		goto free;
	}


	pstr = iniparser_getstring(dict, "server:ip", NULL);
	if (pstr == NULL || isipv4(pstr) < 0 ) {
		iniparser_freedict(dict);
		goto free;
	}

	dest_ip = ipv4touint(pstr);
	
	pstr = iniparser_getstring(dict, "server:port", NULL);
	if (pstr == NULL || isport(pstr) < 0 ) {
		iniparser_freedict(dict);
		goto free;
	}
	snprintf (port_tmp, sizeof(port_tmp),"%s", pstr);
	dest_port = (unsigned short)atoi(pstr);
	
	iniparser_freedict(dict);

	handle->dest.sin_addr.s_addr = htonl(dest_ip);
	handle->dest.sin_port = htons(dest_port);
	handle->dest.sin_family = AF_INET;
	
	handle->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (handle->sock < 0) {
		goto free;
	}

	snprintf (handle->header.module,sizeof(handle->header.module), "%s", module_name);
	handle->header.pid = (unsigned int)getpid();
	handle->header.pid = htonl(handle->header.pid);
	return handle;
free:
	log_exit(handle);
	return NULL;

}

void log_exit(void *handle)
{

	(void)handle;

	return;

}

void log_send(void *handle, int level,const char *format, ...)
{


	struct log_handle *_handle = NULL;
	int copy_size = 0; 
	size_t header_size = 0;
	socklen_t sock_len = 0;
	va_list args;


	_handle = (struct log_handle *)handle;


	pthread_mutex_lock(&_handle->lock);
	sock_len = sizeof(_handle->dest);
	
	_handle->header.level = level;
	header_size = sizeof(_handle->header);
	/*header hton*/

	/**/
	
	copy_size = 0;
	
	memcpy(_handle->buf,&_handle->header, header_size);
	copy_size += header_size;

	/*module name*/
	va_start(args, format);
	copy_size += vsnprintf(_handle->buf + copy_size, PAYLOAD_SIZE - copy_size, format,args);
	va_end(args);
	sendto(_handle->sock, _handle->buf, copy_size, 0, (struct sockaddr*)&_handle->dest,sock_len);

	pthread_mutex_unlock(&_handle->lock);

	return;

}

void log_fatal(void *handle, const char *format, ...)
{
	log_send(handle, _oplog_fatal,format);
	return;

}
void log_error(void *handle ,const char *format, ...)
{
	log_send(handle, _oplog_error,format);
	return;

}
void log_warn(void *handle, const char *format, ...)
{
	log_send(handle, _oplog_warn,format);
	return;
}
void log_info(void *handle, const char *format, ...)
{
	log_send(handle, _oplog_info,format);
	return;
}
void log_debug(void *handle, const char *format, ...)
{

	log_send(handle, _oplog_debug,format);
	return;
}



