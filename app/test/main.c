#include "interface/module.h"
#include "interface/log.h"

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;

	void *h = log_init(MODULE_OPCLI);
	while(1)
	log_info(h, "hello world\n");

	return 0;
}
