#include "u9300c.h"
#include "uart.h"
#include <pthread.h>
#include "cell_module.h"
#include "cell_pub.h"
#include "at_cmd.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

struct lte_module_info *u9300_module = NULL;


static struct lte_module_info *get_u9300c_module(void)
{
	return u9300_module;
}

struct module_at_map u9300c_map[AT_CMD_MAX] = {
	[AT_CMD_TEST] = {.at_cmd="AT\n"},
	[AT_CMD_MESSAGE_TEXT] = {.at_cmd="AT+CMGS=\"%s\"\n%s"},

};

int u9300_init(struct lte_module_info *module)
{
	if (!module)
		goto out;
	
	cell_log_debug("u9300 init\n");
	u9300_module = module;
	u9300_send_short_message("18519127396", "how are you", strlen("how are you"), SHORT_MESSAGE_TEXT);
	return 0;
out:
	return -1;
}

int u9300_send_short_message(char *phone_num, char * message, unsigned short size, int decode_type)
{

	struct lte_module_info *module = NULL;
	char buf_tmp[1024];
	int buf_size = 0;
	int write_size = 0;
	int ret = 0;
	int index = 0;
	if (!(module = get_u9300c_module()) || !phone_num || !message || !size || 
		(decode_type != SHORT_MESSAGE_TEXT && decode_type != SHORT_MESSAGE_PDU))
		return -1;

	write_size = 0;
	buf_size = sizeof(buf_tmp)-2;
	index = 0;
	if (decode_type == SHORT_MESSAGE_TEXT) {
		ret = snprintf(buf_tmp+index, buf_size - index,"%s", "AT$QCMGF=1\n");
		if (ret <= 0)
			return -1;
		index += ret;
		ret = snprintf (buf_tmp+index, buf_size - index, u9300c_map[AT_CMD_MESSAGE_TEXT].at_cmd,phone_num, message);
		if (ret <= 0)
			return -1;
		index += ret;
	}
	write_size = index;
	buf_tmp[index]= 0x1a;
	write_size += 1;
	buf_tmp[index+1]= 0;
	write_size += 1;
	
	pthread_mutex_lock(&module->lock);
	if (module->current_map)
		pthread_cond_wait(&module->cond, &module->lock);
	
	write(module->fd, buf_tmp, write_size);
	pthread_cond_broadcast(&module->cond);
	pthread_mutex_unlock(&module->lock);
	return 0;
}



