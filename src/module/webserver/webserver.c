#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "webserver.h"
#include "nginx.h"
#include "base/oplog.h"
#include "config.h"
#include "opbox/utils.h"

#include "iniparser.h"
#include "base/oplog.h"
#include "opbox/utils.h"

#define WEBSERVER_CONF "webserver:path"
#define WEBSERVER_CONF_PREFIX "webserver:prefix"

void nginx_main_start(char *opserver_conf_path)
{
#define WEBSERVER_ARGV_NUM 5
#define WEBSERVER_ARGV_ELE_LENGTH 128
	const char *str = NULL;
	dictionary *dict = NULL;
	char web_conf_file[246] = {};
	char web_prefix[64] = {};
	int argc =0 ;
	char **argv = NULL;
	int i = 0;

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		log_error ("iniparser_load faild[%s]\n", OPSERVER_CONF);
		return;
	}

	if(!(str = iniparser_getstring(dict,WEBSERVER_CONF,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", WEBSERVER_CONF);
		return;
	}

	snprintf(web_conf_file, sizeof(web_conf_file), "%s/%s", str, "webserver.conf");

	if(!(str = iniparser_getstring(dict,WEBSERVER_CONF,NULL))) {
		log_error_ex ("iniparser_getstring faild[%s]\n", WEBSERVER_CONF);
		return;
	}
	
	snprintf(web_prefix, sizeof(web_prefix), "%s", str);

	iniparser_freedict(dict);

	argv = calloc(1, sizeof(char*) * WEBSERVER_ARGV_NUM);
	if (!argv) {
		log_error_ex ("calloc failed\n");
		return;
	}

	for(i = 0; i < WEBSERVER_ARGV_NUM; i++) {
		argv[i] = calloc(1, WEBSERVER_ARGV_ELE_LENGTH);
		if (!argv[i]) {
			log_error_ex ("calloc failed, index=%d\n",i);
			return;
		}
	}

	strlcpy(argv[0],"nginx", WEBSERVER_ARGV_ELE_LENGTH);
	strlcpy(argv[1],"-c", WEBSERVER_ARGV_ELE_LENGTH);
	strlcpy(argv[2],web_conf_file, WEBSERVER_ARGV_ELE_LENGTH);
	strlcpy(argv[3],"-p", WEBSERVER_ARGV_ELE_LENGTH);
	strlcpy(argv[4],web_prefix, WEBSERVER_ARGV_ELE_LENGTH);

	argc = WEBSERVER_ARGV_NUM;

	log_debug_ex("webserver init...\n");
	nginx_main(argc, argv);
	return;
}

int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	op_daemon();
	nginx_main_start(OPSERVER_CONF);
	return 0;
}

void webserver_exit(void *web)
{
	return;
}
