#include "file.h"
#include "magic.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <wchar.h>
#include "base/oplog.h"
struct _pm{
	const char *name;
	int tag;
	size_t value;
	int set;
	size_t def;
	const char *desc;
};

struct _pm pm[] = {
	{ "bytes",	MAGIC_PARAM_BYTES_MAX, 0, 0, FILE_BYTES_MAX,
	    "max bytes to look inside file" },
	{ "elf_notes",	MAGIC_PARAM_ELF_NOTES_MAX, 0, 0, FILE_ELF_NOTES_MAX,
	    "max ELF notes processed" },
	{ "elf_phnum",	MAGIC_PARAM_ELF_PHNUM_MAX, 0, 0, FILE_ELF_PHNUM_MAX,
	    "max ELF prog sections processed" },
	{ "elf_shnum",	MAGIC_PARAM_ELF_SHNUM_MAX, 0, 0, FILE_ELF_SHNUM_MAX,
	    "max ELF sections processed" },
	{ "encoding",	MAGIC_PARAM_ENCODING_MAX, 0, 0, FILE_ENCODING_MAX,
	    "max bytes to scan for encoding" },
	{ "indir",	MAGIC_PARAM_INDIR_MAX, 0, 0, FILE_INDIR_MAX,
	    "recursion limit for indirection" },
	{ "name",	MAGIC_PARAM_NAME_MAX, 0, 0, FILE_NAME_MAX,
	    "use limit for name/use magic" },
	{ "regex",	MAGIC_PARAM_REGEX_MAX, 0, 0, FILE_REGEX_MAX,
	    "length limit for REGEX searches" },
};

private int process(struct magic_set *ms, const char *, int);
private struct magic_set *load(const char *, int);

private void applyparam(magic_t magic);

int file_main(int argc, char *argv[])
{

	int flags = 0;
	struct magic_set *magic = NULL;
	const char *magicfile = "/home/isir/developer/tmp/file-master/magic/magic.mgc";
	if (!(magic = load(magicfile, flags))) {
		log_warn_ex("file main load failed\n");
		return 1;
	}

	applyparam(magic);
	process(magic, "/home/isir/developer/doc/U9300_1_9x07平台_AT手册_V5.3.9.pdf", 0);

	if (magic)
		magic_close(magic);
	log_warn_ex("file main over\n");
	return 0;
}

private void applyparam(magic_t magic)
{
	size_t i;

	for (i = 0; i < __arraycount(pm); i++) {
		if (!pm[i].set)
			continue;
		if (magic_setparam(magic, pm[i].tag, &pm[i].value) == -1)
			log_warn_ex("Can't set %s", pm[i].name);
	}
}

private struct magic_set *load(const char *magicfile, int flags)
{
	struct magic_set *magic = magic_open(flags);
	const char *e;

	if (!magic) {
		log_warn_ex("Can't create magic\n");
		return NULL;
	}
	
	if (magic_load(magic, magicfile)< 0) {
		magic_close(magic);
		return NULL;
	}
	
	if ((e = magic_error(magic)))
		log_warn_ex("%s", e);
	
	return magic;
}


private int process(struct magic_set *ms, const char *inname, int wid)
{
	const char *type = NULL;

	type = magic_file(ms, inname);

	if (!type)
		log_warn_ex("ERROR: %s\n", magic_error(ms));
	else
		log_debug_ex("%s\n", type);
	return type == NULL;
}

