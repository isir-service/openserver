#include "cell_module.h"
#include "cell_pub.h"
#include "uart.h"

void cell_module_init(void)
{
	struct op_cell *cell = get_cell_self();
	int vendor_id = 0x1c9e;
	int product_id = 0x9b3c;
	int i = 0;
	for (i = 0; i < LTE_MODULE_MAX; i++) {
		if (cell->module[i].vendor_id != vendor_id || cell->module[i].product_id != product_id)
			continue;

		cell->module[i].fd = uart_init(cell->module[i].at_dev);
		if (cell->module[i].fd < 0)
			continue;


		cell->module[i].base = event_base_new();
		if (!cell->module[i].base)
			goto out;
		
		cell->module[i].read = event_new(cell->module[i].base, cell->module[i].fd, EV_READ | EV_PERSIST, uart_read, &cell->module[i]);
		if (!cell->module[i].read)
			goto out;
		
		if(event_add(cell->module[i].read, NULL) < 0)
			goto out;

		cell->module[i].call_init =  evtimer_new(cell->module[i].base, module_call_timer, &cell->module[i]);
		if (!cell->module[i].call_init)
			goto out;

		cell->module[i].call_t.tv_sec = 1;
		if(event_add(cell->module[i].call_init, &cell->module[i].call_t) < 0)
			goto out;

		cell->module[i].read_buf.buf_size = sizeof(cell->module[i].read_buf.buf);
		pthread_mutex_init(&cell->module[i].lock, NULL);
		pthread_cond_init(&cell->module[i].cond,NULL);
		if (pthread_create(&cell->module[i].thread_id, NULL, module_thread_job, cell->module[i].base))
			goto out;

	}

out:
	return;
}

void *module_thread_job(void *arg)
{
	if (!arg)
		goto out;
	
	event_base_loop(arg,EVLOOP_NO_EXIT_ON_EMPTY);
out:
	pthread_join(pthread_self() , NULL);
	return NULL;
}

void module_call_timer(int s, short what, void *arg)
{
	(void)s;
	(void)what;
	struct lte_module_info *module = (struct lte_module_info *)arg;
	if (!module)
		return;

	if (!module->init)
		return;

	if (module->init(module) < 0)
		return;

	module->use = 1;

	return;
}



