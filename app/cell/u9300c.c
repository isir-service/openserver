#include "u9300c.h"
#include "uart.h"
#include <pthread.h>
#include "cell_module.h"
#include "cell_pub.h"
#include "at_cmd.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "libubox/usock.h"

struct lte_module_info *u9300_module = NULL;


static struct lte_module_info *get_u9300c_module(void)
{
	return u9300_module;
}

struct module_at_map u9300c_map[AT_CMD_MAX] = {
	[AT_CMD_TEST] = {.at_cmd="AT\r"},
	[AT_CMD_MESSAGE_TEXT] = {.at_cmd="AT+CMGS=\"%s\"\r"},

};

int u9300_init(struct lte_module_info *module)
{
	if (!module)
		goto out;
	
	cell_log_debug("u9300 init\n");
	u9300_module = module;
	u9300_send_text_message("18519127396", "sail stock 80018", strlen("sail stock 80018"));
	return 0;
out:
	return -1;
}

int u9300_send_text_message(char *phone_num, char * message, unsigned short size)
{

	struct lte_module_info *module = NULL;
	char buf_tmp[1024];
	int ret = 0;
	int defaul_wait_mstime = 500;
	
	if (!(module = get_u9300c_module()) || !phone_num || !message || !size)
		return -1;
	
	pthread_mutex_lock(&module->lock);
	if (module->current_map)
		pthread_cond_wait(&module->cond, &module->lock);

	module->current_map = &u9300c_map[AT_CMD_TEST];
	ret = snprintf(buf_tmp, sizeof(buf_tmp),"%s", "AT$QCMGF=1\r");
	if (ret <= 0)
		goto out;

	if (write (module->fd, buf_tmp, ret) <= 0)
		goto out;

	ret = usock_wait_ready(module->fd, defaul_wait_mstime);
	if (ret <= 0)
		goto out;

	memset(buf_tmp, 0, sizeof(buf_tmp));
	ret = read(module->fd, buf_tmp, sizeof(buf_tmp));
	if (ret <= 0)
		goto out;

	if (!strstr(buf_tmp,"\r\nOK\r\n"))
		goto out;

	ret = snprintf(buf_tmp, sizeof(buf_tmp),"%s", "AT+CSCS=\"GSM\"\r");
	if (ret <= 0)
		goto out;

	if (write (module->fd, buf_tmp, ret) <= 0)
		goto out;

	ret = usock_wait_ready(module->fd, defaul_wait_mstime);
	if (ret <= 0)
		goto out;

	memset(buf_tmp, 0, sizeof(buf_tmp));
	ret = read(module->fd, buf_tmp, sizeof(buf_tmp));
	if (ret <= 0)
		goto out;

	if (!strstr(buf_tmp,"\r\nOK\r\n"))
		goto out;


	ret = snprintf (buf_tmp, sizeof(buf_tmp), u9300c_map[AT_CMD_MESSAGE_TEXT].at_cmd, phone_num);
	if (ret <= 0)
		goto out;

	if (write (module->fd, buf_tmp, ret) <= 0)
		goto out;

	ret = usock_wait_ready(module->fd, defaul_wait_mstime);
	if (ret <= 0)
		goto out;

	memset(buf_tmp, 0, sizeof(buf_tmp));
	ret = read(module->fd, buf_tmp, sizeof(buf_tmp));
	if (ret <= 0)
		goto out;

	ret = snprintf (buf_tmp, sizeof(buf_tmp)-1, "%s", message);
	if (ret <= 0)
		goto out;

	buf_tmp[ret] = 0x1A;
	buf_tmp[ret+1] = 0;
	if (write (module->fd, buf_tmp, ret+1) <= 0)
		goto out;

	ret = usock_wait_ready(module->fd, defaul_wait_mstime);
	if (ret <= 0)
		goto out;

	memset(buf_tmp, 0, sizeof(buf_tmp));
	ret = read(module->fd, buf_tmp, sizeof(buf_tmp));
	if (ret <= 0)
		goto out;

	if (!strstr(buf_tmp,"\r\nOK\r\n"))
		goto out;
	
	module->current_map = NULL;
	pthread_cond_broadcast(&module->cond);
	pthread_mutex_unlock(&module->lock);
	return 0;

out:
	
	module->current_map = NULL;
	pthread_cond_broadcast(&module->cond);
	pthread_mutex_unlock(&module->lock);
	return -1;
}

int u9300_send_pdu_message(char *phone_num, char *phone_center, char * message, unsigned short size)
{
	(void)phone_num;
	(void)phone_center;
	(void)message;
	(void)size;

	return 0;
}


