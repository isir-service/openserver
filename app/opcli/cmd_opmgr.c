#include "cmd_opmgr.h"
#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include "interface/bus.h"
#include "interface/mgr.h"
#include "opcli.h"
#include "interface/module.h"


DEFUN (opmgr_show_meminfo_func,
		opmgr_show_meminfo_cmd,
		"show meminfo",
		"show\n"
		"meminfo")
{
	(void)self;
	(void)argc;
	(void)argv;
	(void)vty;

	bus_send_sync(get_opcli_bus(), 0, MODULE_OPMGR, mgr_meminfo, NULL, 0, vty->recv_buf, sizeof(vty->recv_buf), NULL, 1000);
	vty_out(vty,"%s\r\n", vty->recv_buf);
	
	return CMD_SUCCESS;
}

DEFUN (opmgr_show_cpu_usage_func,
		opmgr_show_cpu_usage_cmd,
		"show cpu_usage",
		"show\n"
		"cpu_usage")
{
	(void)self;
	(void)argc;
	(void)argv;
	(void)vty;

	bus_send_sync(get_opcli_bus(), 0, MODULE_OPMGR, mgr_cpu_usage, NULL, 0, vty->recv_buf, sizeof(vty->recv_buf), NULL, 1000);
	vty_out(vty,"%s\r\n", vty->recv_buf);
	
	return CMD_SUCCESS;
}


void cmd_install_opmgr(void)
{

	install_element(OPMGR_NODE, &opmgr_show_meminfo_cmd);
	
	install_element(OPMGR_NODE, &opmgr_show_cpu_usage_cmd);
	return;
}

