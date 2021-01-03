#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "u9300.h"
#include "base/oplog.h"
#include "opbox/list.h"

struct _u9300_struct {
	int fd;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	struct list_head list;
	struct _4g_cmd *current;
};

static struct _u9300_struct *self = NULL;

static int u9300_cmd_vendor_name(struct _4g_cmd *cmd, char *resp, int resp_size)
{

	return 0;
}

static int u9300_cmd_module_type(struct _4g_cmd *cmd, char *resp, int resp_size)
{

	return 0;
}

struct _4g_cmd u9300_cmd [_4G_CMD_MAX] = {
	[_4G_CMD_AT_TEST] = {.at = "AT\r", .at_cmd_cb = NULL},
	[_4G_CMD_VENDOR_NAME] = {.at = "AT+CGMI\r", .at_cmd_cb = u9300_cmd_vendor_name},
	[_4G_CMD_MODULE_TYPE] = {.at = "AT+CGMM\r", .at_cmd_cb = u9300_cmd_module_type},
	[_4G_CMD_MODULE_IMEI] = {.at = "AT+CGSN\r", .at_cmd_cb = NULL},
};

static void u9300_add_cmd(unsigned int cmd)
{
	if (cmd >= _4G_CMD_MAX || !u9300_cmd[cmd].at) {
		log_warn("cmd not support[cmd=%d]\n", cmd);
		goto exit;
	}

	list_add_tail(&u9300_cmd[cmd].list, &self->list);

exit:
	return;
}

static int u9300_write_cmd(void)
{
	struct _4g_cmd *item = NULL;
	pthread_mutex_lock(&self->lock);
	if (list_empty(&self->list))
		goto out;

	item = list_first_entry(&self->list, struct _4g_cmd , list);
	self->current = item;
	log_debug("4g write[%d] cmd=%s\n" ,self->fd, item->at);
	if (write(self->fd, item->at, strlen(item->at)) < 0) {
		log_warn("write failed [cmd=%s][errno]\n", item->at, errno);
		goto out;
	}

out:
	pthread_mutex_unlock(&self->lock);
	return 0;
}

int u9300_init(int fd)
{
	struct _u9300_struct *u = NULL;
	int i = 0;
	int len = 0;

	log_debug("u9300 init\n");

	u = calloc(1, sizeof(*u));
	if (!u) {
		log_error("calloc failed[%d]\n", errno);
		goto exit;
	}
	
	u->fd = fd;

	self = u;

	if(pthread_mutexattr_init(&u->attr)) {
		log_error ("pthread_mutexattr_init faild\n");
		goto exit;
	}

	if(pthread_mutex_init(&u->lock, &u->attr)) {
		log_error ("pthread_mutex_init faild\n");
		goto exit;
	}


	INIT_LIST_HEAD(&u->list);

	len = sizeof(u9300_cmd)/sizeof(u9300_cmd[0]);

	for (i = 0; i < len; i++)
		INIT_LIST_HEAD(&u9300_cmd[i].list);


	pthread_mutex_lock(&u->lock);
	
	u9300_add_cmd(_4G_CMD_AT_TEST);
	u9300_add_cmd(_4G_CMD_VENDOR_NAME);
	u9300_add_cmd(_4G_CMD_MODULE_TYPE);
	u9300_add_cmd(_4G_CMD_MODULE_IMEI);
	pthread_mutex_unlock(&u->lock);

	u9300_write_cmd();

	return 0;

exit:
	u9300_exit();
	return -1;
}

int u9300_handle(int fd, unsigned int event_type, struct _4g_handle *handle)
{
	struct _4g_cmd *item = NULL;

	pthread_mutex_lock(&self->lock);
	if (list_empty(&self->list) || !self->current)
		goto NONE;

	log_debug("u9300 resp: %s\n", handle->req);

	if (!strstr((char*)handle->req,"\r\n"))
		goto MORE;
	
	list_del(&self->current->list);

	if (list_empty(&self->list))
		goto NONE;

	item = list_first_entry(&self->list, struct _4g_cmd , list);

	self->current = item;
	
	log_debug("4g write[%d] cmd=%s\n" ,self->fd, item->at);
	if (write(self->fd, item->at, strlen(item->at)) < 0) {
		log_warn("write failed [cmd=%s][errno]\n", item->at, errno);
		goto NONE;
	}

	pthread_mutex_unlock(&self->lock);
	return _4G_EVENT_NEXT;
NONE:
	self->current = NULL;
	pthread_mutex_unlock(&self->lock);
	return _4G_EVENT_NONE;
MORE:
	pthread_mutex_unlock(&self->lock);
	return _4G_EVENT_MORE;
}

void u9300_exit(void)
{
	return;
}


