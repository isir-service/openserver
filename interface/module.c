#include "module.h"

struct module_name_map {
	char *name;
};

struct module_name_map module_map[MODULE_MAX] = {
	[MODULE_OPCLI] = {.name = "opcli"},
	[MODULE_OPBUS] = {.name = "opbus"},
};

char *module_id_to_name(unsigned int module_id)
{
	if (module_id >= MODULE_MAX)
		return "";

	return module_map[module_id].name;

}


