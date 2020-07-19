#include "cell.h"
#include <stdlib.h>
#include "cell_pub.h"

struct op_cell *cell_init(void)
{
	struct op_cell *cell = calloc(1, sizeof(struct op_cell));
	if (!cell)
		goto out;

	cell->module = lte_module;

	lte_self = cell;
	return cell;
out:
	cell_exit(cell);
	return NULL;
}

void cell_exit(struct op_cell *cell)
{
	(void)cell;
	return;
}

void cell_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)h;
	(void)from_module;
	(void)from_sub_id;
	(void)to_sub_id;
	(void)data;
	(void)size;
	(void)arg;

	return;

}

void cell_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}

