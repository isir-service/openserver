#include <cli/thread.h>

#include <cli/vty.h>
#include <cli/command.h>
#include <cli/vtysh.h>
#include <stdlib.h>
#include <sys/types.h>

static char *line_vty_read = NULL;


int main(int argc, char *argv[])

{

	(void)argc;
	(void)argv;

	vtysh_init();

	/*install element*/
	
	sort_node ();
	vtysh_readline_init();

	while (vtysh_rl_gets (&line_vty_read))
	  vtysh_execute (line_vty_read);

	return 0;

}
