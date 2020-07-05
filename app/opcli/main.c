#include <cli/thread.h>

#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include <stdlib.h>
#include <sys/types.h>
#include "opcli.h"
#include "interface/module.h"
#include "cmd_opbus.h"
#include "interface/log.h"
#include "cmd_opmgr.h"
#include <signal.h>

int main(int argc, char *argv[])
{

	(void)argc;
	(void)argv;

	struct opcli *cli = opcli_init();
	if (!cli)
		goto out;

	cli->log = log_init(MODULE_OPCLI);
	if (!cli->log)
		goto out;

	
	log_debug(cli->log, "opcli init\n");
	
	cli->master = thread_master_create();
	if (!cli->master)
		goto out;




	cli->bus = bus_connect(MODULE_OPCLI, opcli_bus_cb, opcli_bus_disconnect, NULL);
	if (!cli->bus)
		goto out;
	vty_init(cli->master);

	cmd_init();

	/*install element*/
	cmd_install_opbus();
	cmd_install_opmgr();

	sort_node ();
	vty_serv_sock("127.0.0.1",55555,NULL);
	
	thread_loop(cli->master);

	return 0;

out:
	opcli_exit(cli);
	return 0;
}
