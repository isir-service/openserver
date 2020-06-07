#include "libubox/log.h"


int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;

	void *h = log_init(LOG_MODULE_OPCLI);
	while(1)
	log_info(h, "hello world\n");

	return 0;
}
