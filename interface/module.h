#ifndef __MODULE_H__
#define __MODULE_H__

enum {
	MODULE_OPCLI,
	MODULE_OPBUS,
	MODULE_MAX,
};

char *module_id_to_name(unsigned int module_id);

#endif

