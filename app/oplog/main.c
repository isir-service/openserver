#include "oplog.h"
#include "event.h"
#include "libubox/usock.h"
#include <stdio.h>
#include "interface/module.h"
#include "interface/bus.h"
#include "interface/log.h"
#include "libubox/utils.h"
int main(int argc ,char **argv)
{
	(void)argc;
	(void)argv;

	struct oplog *op_log = oplog_init();
	if (!op_log)
		goto out;

	if(oplog_parse_conf(LOG_CONF_PATH) < 0)
		goto out;

	op_log_apply_conf();

	op_log->timeout.tv_sec = 10;

	op_log->ebase =  event_base_new();
	if (!op_log->ebase)
		goto out;

	op_log->bus = bus_connect(MODULE_OPLOG, oplog_bus_cb, oplog_bus_disconnect, NULL);
	if (!op_log->bus)
		goto out;

	op_log->udp_fd = usock(USOCK_SERVER|USOCK_UDP, "127.0.0.1", usock_port(op_log->conf.port));
	if (op_log->udp_fd <= 0)
		goto out;

	op_log->log = log_init(MODULE_OPLOG);
	if (!op_log->log)
		goto out;

	op_log->tm = event_new(op_log->ebase, -1, EV_PERSIST, oplog_timer, op_log);
	if (event_add(op_log->tm, &op_log->timeout) < 0)
		goto out;

	op_log->read = event_new(op_log->ebase, op_log->udp_fd, EV_READ | EV_PERSIST, oplog_read, op_log);
	if (!op_log->read)
		goto out;

	if(event_add(op_log->read, NULL) < 0)
		goto out;

	event_base_dispatch(op_log->ebase);

out:
	oplog_exit(op_log);
	return -1;
}
