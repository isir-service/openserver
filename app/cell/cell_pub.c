#include "cell_pub.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "u9300c.h"

struct lte_module_info lte_module[LTE_MODULE_MAX] = {
	[LTE_MODULE_U9300C] = {.vendor_id=0x1c9e, .product_id=0x9b3c, .at_dev="/dev/ttyUSB1", .init=u9300_init},
};

struct op_cell *get_cell_self(void)
{
	return lte_self;
}

void lte_release_module(struct lte_module_info *module)
{
	if (!module)
		return;

	if (module->base) {
		event_base_free(module->base);
		module->base = NULL;
	}

	if (module->fd > 0 ) {
		close(module->fd);
		module->fd = 0;
	}

	module->use = 0;
	
	return;
}


