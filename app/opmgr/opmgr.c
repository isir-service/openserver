#include "opmgr.h"
#include <stdlib.h>
#include <assert.h>
#include "interface/mgr.h"
#include "opmgr_bus_cb.h"


struct opmgr *opmgr_self = NULL;
struct opmgr *get_h_opmgr(void)
{
	assert(opmgr_self);

	return opmgr_self;
}


struct opmgr *opmgr_init(void)
{
	struct opmgr *mgr = NULL;
	
	mgr = calloc(1, sizeof(struct opmgr));
	if (!mgr)
		goto out;
	
	opmgr_self = mgr;
	return mgr;

out:
	opmgr_exit(mgr);
	return NULL;
}

void opmgr_exit(struct opmgr *mgr)
{
	(void)mgr;

	return;
}

void opmgr_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id, unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)data;
	(void)size;
	(void)arg;
	int res_size = 0;
	
	struct opmgr *mgr = get_h_opmgr();
	
	if (to_sub_id >= mgr_max)
		return;

	if((res_size = handle_mgr_sub_id(from_module, to_sub_id, mgr->bus_buf, sizeof(mgr->bus_buf))) < 0)
		return;

	bus_send(h, to_sub_id, from_module, from_sub_id, mgr->bus_buf, res_size);

	return;
}
void opmgr_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}

