#ifndef _OPFILE_H__
#define _OPFILE_H__

struct file_info {
	char ext[64];
	char desc[128];
	unsigned type_id;
};

void *opfile_init(void);
void opfile_exit(void *file);
struct file_info * opfile_check_mem(char *file_buf, unsigned int size);
struct file_info * opfile_check_path(char *file_path);

#endif
