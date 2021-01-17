#include <stdio.h>
#include <unistd.h>
#include<getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "base/opbus_type.h"
#include "opmgr_bus.h"

#include "config.h"
#include "iniparser.h"
#include "opbox/utils.h"
#include "opbox/usock.h"

#define PBUS_PATH "env:path"
#define PBUS_LIB "env:lib"

typedef int (*pbus_send)(unsigned int type, unsigned char *req, int size, unsigned char *response,int res_size);
typedef int (*pbus_format_res)(unsigned int type, unsigned char *response,int res_size);


int pbus_env_set(void)
{
	dictionary *dict;
	const char *str;
	char buf[2048] = {};
	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FILE__,__LINE__, OPSERVER_CONF);
		goto out;
	}

	if(!(str = iniparser_getstring(dict,PBUS_PATH,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, PBUS_PATH);
		iniparser_freedict(dict);
		goto out;
	}

	snprintf(buf, sizeof(buf), "%s:%s", getenv("PATH"), str);

	setenv("PATH", buf, 1);

	if(!(str = iniparser_getstring(dict,PBUS_LIB,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, PBUS_LIB);
		iniparser_freedict(dict);
		goto out;
	}
	snprintf(buf, sizeof(buf), "%s:%s", getenv("LD_LIBRARY_PATH"), str);

	setenv("LD_LIBRARY_PATH", buf, 1);
	
	return 0;
out:
	return -1;
}

int format_cpu_usage(unsigned int type,unsigned char *response,int res_size)
{
	struct cpu_info *cpu_usage = NULL;
	int i = 0;
	cpu_usage = (struct cpu_info *)response;
	printf("name      %%user       %%nice       %%system      %%iowait      %%steal      %%usage\n");

	for(i = 0; i <= cpu_usage->cpu_num;i++) {
		printf("%-6s    %-6.2f      %-6.2f      %-7.2f      %-7.2f      %-6.2f      %-6.2f\n",
				cpu_usage->usage[i].cpu_name, cpu_usage->usage[i].user,cpu_usage->usage[i].nice, cpu_usage->usage[i].system,
				cpu_usage->usage[i].iowait, cpu_usage->usage[i].steal, cpu_usage->usage[i].cpu_use);
	}

	return 0;

}

int pbus_send_busd(unsigned int type, unsigned char *req, int size, unsigned char *response,int res_size)
{

	int fd = 0;
	struct _bus_req_head *head = NULL;
	struct _bus_response_head *response_head = NULL;
	int ret = 0;
	int count = 0;
	const char *str = NULL;
	char *res = NULL;
	dictionary * dict = NULL;
	char ip[16];
	int port;
	char *buf_req = NULL;
	buf_req = calloc(1, _BUS_BUF_REQ_SIZE);
	if (!buf_req) {
		printf ("%s %d calloc faild[%d]\n", __FUNCTION__, __LINE__,errno);
		goto failed;
	}
	
	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FUNCTION__, __LINE__ ,OPSERVER_CONF);
		goto failed;
	}

	if(!(str = iniparser_getstring(dict,BUS_SERVER,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n", __FUNCTION__, __LINE__, BUS_SERVER);
		iniparser_freedict(dict);
		goto failed;
	}

	strlcpy(ip, str, sizeof(ip));
	if ((port =iniparser_getint(dict,BUS_PORT,-1)) < 0) {
		printf ("%s %d iniparser_getint faild[%s]\n", __FUNCTION__, __LINE__, BUS_PORT);
		iniparser_freedict(dict);
		goto failed;
	}

	iniparser_freedict(dict);

	if (type >= opbus_max) {
		printf ("%s %d faild[type = %u, size= %d]\n", __FUNCTION__, __LINE__, type, size);
		goto failed;
	}

	fd = usock(USOCK_IPV4ONLY|USOCK_TCP, ip, usock_port(port));
	if (fd < 0) {
		printf ("%s %d usock faild[%d]\n", __FUNCTION__, __LINE__, errno);
		goto failed;
	}

	head = (struct _bus_req_head *)buf_req;
	head->type = htonl(type);
	if (size > (int)(_BUS_BUF_REQ_SIZE-sizeof(*head))) {
		printf ("%s %d faild[size= %d，support= %d]\n", __FUNCTION__, __LINE__, size, (int)(_BUS_BUF_REQ_SIZE-sizeof(*head)));
		goto failed;
	}

	if (req && size)
		memcpy(buf_req+sizeof(*head), req, size);
	if (write(fd, buf_req, sizeof(*head)+size) < 0) {
		printf ("%s %d write[%d]\n", __FUNCTION__, __LINE__, errno);
		goto failed;
	}


	ret = usock_wait_ready(fd, _BUS_WAIT_MS);
	if (ret <= 0) {
		printf ("%s %d wait timeout[type=%u]\n", __FUNCTION__, __LINE__, type);
		goto failed;
	}

	res = calloc(1, _BUS_BUF_RESPONSE_SIZE);
	if (!res) {
		printf ("%s %d calloc failed[type=%u][%d]\n",__FUNCTION__, __LINE__, type, errno);
		goto failed;
	}

	ret = read(fd, res, _BUS_BUF_RESPONSE_SIZE);
	if (ret < 0) {
		free(res);
		printf ("%s %d read failed[type=%u][%d]\n",__FUNCTION__, __LINE__, type, errno);
		goto failed;
	}

	if (ret < (int)sizeof(*response_head)) {
		printf ("%s %d too short[%d]\n", __FUNCTION__, __LINE__, ret);
		free(res);
		goto failed;
	}

	response_head = (struct _bus_response_head *)res;

	if (response && res_size > 0) {
		if (res_size < (int)(ret-sizeof(*response_head)))
			printf ("%s %d copy truncate[respose=%d]\n", __FUNCTION__, __LINE__, (int)(ret-sizeof(*response_head)));

		count = res_size > (int)(ret-sizeof(*response_head))? (int)(ret-sizeof(*response_head)):res_size;
		
		memcpy(response, res+sizeof(*response_head), count);
	}

	close(fd);

	free(res);

	return count;

failed:
	if (fd)
		close(fd);

	if (buf_req)
		free(buf_req);

	return -1;
}

struct _pbus_struct {
	char *help;
	int use_req;
	pbus_send cb;
	pbus_format_res format_cb;
};

struct _pbus_struct pbus_map [opbus_max] = {
	[opbus_opmgr_get_cpu_usage] = {.help = "get_cpu_usage", .cb = pbus_send_busd, .format_cb = format_cpu_usage},
	[opbus_op4g_send_quotes] = {.help = "send_quotes", .cb = pbus_send_busd},
};

static char *pbus_string = "h";

static struct option pbus_long_options[] =
{
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0},
};

int pbus_help(char *prog)
{
	int i = 0;
	char buf_tmp[1024];

	printf ("usage:\n");
	for (i = 0; i < opbus_max; i++) {
		if (!pbus_map[i].help)
			continue;
		if (pbus_map[i].use_req)
			snprintf (buf_tmp, sizeof(buf_tmp), "%s %s [req string]\n", prog, pbus_map[i].help);
		else
			snprintf (buf_tmp, sizeof(buf_tmp), "%s %s\n", prog, pbus_map[i].help);

		printf("%s",buf_tmp);
	}

	return 0;
}

void pbus_handle (char *help, unsigned char *req, int req_size) {
	int i = 0;
	int ret = 0;

	unsigned char *response = NULL;
	response = calloc(1, _BUS_BUF_RESPONSE_SIZE);
	if (!response) {
		printf("pubus handle calloc failed\n");
		return;
	}
	
	for (i = 0; i < opbus_max; i++) {
		if (!pbus_map[i].help || strncmp(pbus_map[i].help, help, strlen(pbus_map[i].help)))
			continue;

		ret = pbus_map[i].cb(i, req, req_size, response, _BUS_BUF_RESPONSE_SIZE);
		if (ret  < 0) {
			printf("pbus called failed\n");
			return;
		}

		if (pbus_map[i].format_cb)
			pbus_map[i].format_cb(i, response, ret);

		break;
	}

	free(response);
	return;

}

int main(int argc, char *argv[])
{
	int long_index = 0;
	int c = 0;
	pbus_env_set();

	while((c = getopt_long(argc, argv, pbus_string, pbus_long_options, &long_index)) > 0) {
		switch(c) {
			case 'h':
				return pbus_help(argv[0]);
			default:
			break;
		}
	}
	if (argc == 1)
		return pbus_help(argv[0]);

	if (argc > 2)
		pbus_handle(argv[1], (unsigned char*)argv[2], strlen(argv[2]));
	else
		pbus_handle(argv[1], NULL, 0);
	return 0;
}
