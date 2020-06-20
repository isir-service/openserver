#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include "opcli.h"
#include "interface/module.h"

DEFUN (opbus_show_cfg_func,
		opbus_show_cfg_cmd,
		"show opbus config",
		"show\n"
		"opbus module\n"
		"config")
{
	(void)self;
	(void)argc;
	(void)argv;
	(void)vty;
	return CMD_SUCCESS;
}


void cmd_install_opbus(void)
{

	install_element(SHOW_NODE, &opbus_show_cfg_cmd);

	return;
}

