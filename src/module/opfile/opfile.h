#ifndef _OPFILE_H__
#define _OPFILE_H__
enum {
	FILE_TYPE_unknow = 0,
	FILE_TYPE_acoro,
	FILE_TYPE_pdf,
	FILE_TYPE_zip,
	FILE_TYPE_max,
};

struct file_info {
	unsigned int file_type;
	char ext[64];
	char desc[256];
};

void *opfile_init(void);
void opfile_exit(void *file);
struct file_info * opfile_check_mem(char *file_buf, unsigned int size);
struct file_info * opfile_check_path(char *file_path);

#endif
