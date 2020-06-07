#include <cli/thread.h>

#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include <stdlib.h>
#include <sys/types.h>

struct thread_master * master = NULL;
int main(int argc, char *argv[])
{

	(void)argc;
	(void)argv;

	master = thread_master_create();
	if (!master)
		return -1;

	vty_init(master);

	cmd_init();

	/*install element*/

	sort_node ();
	vty_serv_sock(NULL,55555,NULL);
	
	thread_loop(master);

}
