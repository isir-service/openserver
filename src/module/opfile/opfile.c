#include <stdio.h>

#include "opfile.h"
#include "file.h"

void *opfile_init(void)
{
	file_main(0, NULL);

	return NULL;
}

void opfile_exit(void *file)
{
	if (!file)
		return;

	return;
}

