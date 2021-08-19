#include <stdio.h>

#include "opfile.h"
#include "file.h"
#include "base/oplog.h"

void *opfile_init(void)
{
	file_main(0, NULL);

	log_debug_ex("compile magic\n");
	compile_main("/home/isir/developer/tmp/file-master/magic/Magdir", NULL);

	return NULL;
}

void opfile_exit(void *file)
{
	if (!file)
		return;

	return;
}

