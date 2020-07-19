#ifndef __MODULE_H__
#define __MODULE_H__

enum {
	MODULE_OPCLI = 1,
	MODULE_OPBUS,
	MODULE_OPWEB,
	MODULE_OPLOG,
	MODULE_OPMGR,
	MODULE_CELL,
	MODULE_MAX,
};

#define MODULE_NAME_SIZE 120
char *module_id_to_name(unsigned int module_id);

#endif

