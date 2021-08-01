#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "mqtt.h"
#include "mosquitto_ex.h"
#include "base/opmem.h"
#include "base/oplog.h"
#include "opbox/utils.h"
#include "iniparser.h"

#define MQTT_CONF "mqtt:path"

struct _mqtt_struct {
	void *mqtt;
	int mosquitto_pid;
};

static struct _mqtt_struct *self = NULL;

void mqtt_main_start(char *opserver_conf_path)
{
#define MQTT_ARGV_NUM 3
#define MQTT_ARGV_ELE_LENGTH 128
	const char *str = NULL;
	dictionary *dict = NULL;
	char mqtt_conf_file[246] = {};
	int argc =0 ;
	char **argv = NULL;
	int i = 0;

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		log_error_ex ("iniparser_load faild[%s]\n", OPSERVER_CONF);
		return;
	}

	if(!(str = iniparser_getstring(dict,MQTT_CONF,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", MQTT_CONF);
		return;
	}

	snprintf(mqtt_conf_file, sizeof(mqtt_conf_file), "%s/%s", str, "mqtt.conf");

	iniparser_freedict(dict);

	argv = calloc(1, sizeof(char*) * MQTT_ARGV_NUM);
	if (!argv) {
		log_error ("calloc failed\n");
		return;
	}

	for(i = 0; i < MQTT_ARGV_NUM; i++) {
		argv[i] = calloc(1, MQTT_ARGV_ELE_LENGTH);
		if (!argv[i]) {
			log_error_ex ("calloc failed, index=%d\n",i);
			return;
		}
	}

	strlcpy(argv[0],"mqtt", MQTT_ARGV_ELE_LENGTH);
	strlcpy(argv[1],"-c", MQTT_ARGV_ELE_LENGTH);
	strlcpy(argv[2],mqtt_conf_file, MQTT_ARGV_ELE_LENGTH);

	argc = MQTT_ARGV_NUM;

	log_debug("mqtt init...\n");
	mqtt_main(argc, argv);
	return;
}

void *mqtt_init(void)
{
	struct _mqtt_struct *mqtt = NULL;
	int pid = 0;

	mqtt = op_calloc(1, sizeof(*mqtt));
	if (!mqtt) {
		log_error("calloc failed[%d]\n", errno);
		goto exit;
	}

	self = mqtt;

	pid = fork();
	if (pid < 0) {
		log_error("fork failed[%d]\n", errno);
		goto exit;
	}

	if (!pid)
		mqtt_main_start(OPSERVER_CONF);

	log_info("mqtt fork pid[%d]\n", (int)pid);
	mqtt->mosquitto_pid = pid;
	return mqtt;
exit:
	mqtt_exit(mqtt);
	return NULL;
}
void mqtt_exit(void *mqtt)
{
	if (!mqtt)
		return;

	return;
}

