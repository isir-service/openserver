#include <stdio.h>

#include "opfile.h"
#include "file.h"
#include "base/oplog.h"
#include "base/opmem.h"

struct opfile_info {
	struct magic_set *magic;
};

static struct opfile_info *self = NULL;

void *opfile_init(void)
{
	struct opfile_info *opfile = NULL;
	opfile = op_calloc(1, sizeof(struct opfile_info));

	struct file_info *file = NULL;/*test*/
	if (!opfile) {
		log_warn_ex("op calolc failed\n");
		goto out;
	}
	
	opfile->magic = file_init("/home/isir/developer/build/Magdir.mgc");
	if (!opfile->magic) {
		log_warn_ex("file init failed\n");
		goto out;
	}

	self = opfile;

	file = opfile_check_path("/home/isir/developer/build/dd.pdf");

	log_debug_ex("magic: file_name:dd.pdf,file ext:%s, desc:%s\n", file->ext, file->desc);
	//compile_main("/home/isir/developer/tmp/file-master/magic/Magdir", NULL);

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
	const char *desc = NULL;
	struct file_info *file = NULL;
	struct magic magic_info;
	memset(&magic_info, 0, sizeof(magic_info));
	desc = file_magic_check(self->magic, file_path,&magic_info);
	if (!desc)
		return NULL;
	file = op_calloc(1, sizeof(struct file_info));
	if (!file){
		log_warn_ex("opcalloc failed\n");
		return NULL;
	}

	strlcpy(file->ext, magic_info.ext,sizeof(file->ext));
	strlcpy(file->desc, desc, sizeof(file->desc));
	return file;
}

