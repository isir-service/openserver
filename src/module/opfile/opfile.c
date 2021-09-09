#include <stdio.h>

#include "opfile.h"
#include "base/oplog.h"
#include "base/opmem.h"

struct opfile_info {
	int a;
};

static struct opfile_info *self = NULL;

void *opfile_init(void)
{
	struct opfile_info *opfile = NULL;
	opfile = op_calloc(1, sizeof(struct opfile_info));

	if (!opfile) {
		log_warn_ex("op calolc failed\n");
		goto out;
	}

	self = opfile;

	return opfile;
out:
	opfile_exit(opfile);
	return NULL;
}

void opfile_exit(void *file)
{
	if (!file)
		return;

	return;
}

struct file_info * opfile_check_mem(char *file_buf, unsigned int size)
{
	return NULL;
}

struct file_info * opfile_check_path(char *file_path)
{

	return NULL;
}

