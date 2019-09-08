#include "webserver_job.h"
#include <assert.h>
#include <oplog/oplog.h>
#include <libubox/usock.h>
#include <libubox/utils.h>
#include <libxml/tree.h>

#define WEB_THREAD_NUM 10

#define WEB_CONF "../etc/webserver.xml"

struct webserver_job *web_job  = NULL;


int web_get_bind_ip(char *buf,int buf_size)
{

	(void)buf;
	(void)buf_size;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	if (buf_size <= 0) {
		return -1;
	}
	
	doc = xmlReadFile(WEB_CONF,"utf-8", XML_PARSE_NOBLANKS);
	if (doc == NULL) {
		goto exit;
	}
	
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		goto exit;
	}

	
	

exit:
	if (doc) {
		xmlFreeDoc(doc);
	}
	return -1;

}

int web_get_bind_http_port(char *buf, int buf_size)
{
	(void)buf;
	(void)buf_size;

	return 0;

}

int web_get_bind_https_port(char *buf, int buf_size)
{
	(void)buf;
	(void)buf_size;

	return 0;

}

struct webserver_job * webserver_init(void)
{
	void *mem_handle = op_memery_init();
	char bind_ip [40];
	char bind_port_http[20];
	char bind_port_https[20];
	(void)bind_ip;
	(void)bind_port_http;
	(void)bind_port_https;

	

	if (mem_handle == NULL) {
		return NULL;
	}
	
	struct webserver_job *job = op_calloc(mem_handle, 1, sizeof(struct webserver_job));
	if (job == NULL) {
		op_memery_exit(mem_handle);
		return NULL;
	}
	job->mem_handle = mem_handle;
	job->log_handle = log_init("webserver");
	
	if (job->log_handle == NULL) {
		goto exit;
	}

	log_info(job->log_handle, "webserver init\n");
	
	return job;

exit:
	
	webserver_exit(job);
	return NULL;
}

void webserver_exit(struct webserver_job *job)
{
	(void)job;
	return;

}

void webserver_run(struct webserver_job *job)
{
	(void)job;
	
	return;
}

void webserver_set_base (struct webserver_job *handle)
{
	if (web_job) {
		webserver_exit(web_job);
	}

	web_job = handle;

}

struct webserver_job *webserver_get_base (void)
{

	assert(web_job);

	return web_job;
}


