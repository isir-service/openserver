
#include "file.h"


#include "magic.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
/* Since major is a function on SVR4, we cannot use `ifndef major'.  */

#include <sys/sysmacros.h>

private int
handle_mime(struct magic_set *ms, int mime, const char *str)
{
	if ((mime & MAGIC_MIME_TYPE)) {
		if (file_printf(ms, "inode/%s", str) == -1)
			return -1;
		if ((mime & MAGIC_MIME_ENCODING) && file_printf(ms,
		    "; charset=") == -1)
			return -1;
	}
	if ((mime & MAGIC_MIME_ENCODING) && file_printf(ms, "binary") == -1)
		return -1;
	return 0;
}

protected int
file_fsmagic(struct magic_set *ms, const char *fn, struct stat *sb)
{
	int ret, did = 0;
	int mime = ms->flags & MAGIC_MIME;
	int silent = ms->flags & (MAGIC_APPLE|MAGIC_EXTENSION);


	if (fn == NULL)
		return 0;

#define COMMA	(did++ ? ", " : "")
	/*
	 * Fstat is cheaper but fails for files you don't have read perms on.
	 * On 4.2BSD and similar systems, use lstat() to identify symlinks.
	 */

	ret = stat(fn, sb);	/* don't merge into if; see "ret =" above */


	if (ret) {
		if (ms->flags & MAGIC_ERROR) {
			file_error(ms, errno, "cannot stat `%s'", fn);
			return -1;
		}
		if (file_printf(ms, "cannot open `%s' (%s)",
		    fn, strerror(errno)) == -1)
			return -1;
		return 0;
	}

	ret = 1;

	switch (sb->st_mode & S_IFMT) {
	case S_IFDIR:
		if (mime) {
			if (handle_mime(ms, mime, "directory") == -1)
				return -1;
		} else if (silent) {
		} else if (file_printf(ms, "%sdirectory", COMMA) == -1)
			return -1;
		break;
	/* TODO add code to handle V7 MUX and Blit MUX files */


	case S_IFREG:
		/*
		 * regular file, check next possibility
		 *
		 * If stat() tells us the file has zero length, report here that
		 * the file is empty, so we can skip all the work of opening and
		 * reading the file.
		 * But if the -s option has been given, we skip this
		 * optimization, since on some systems, stat() reports zero
		 * size for raw disk partitions. (If the block special device
		 * really has zero length, the fact that it is empty will be
		 * detected and reported correctly when we read the file.)
		 */
		if ((ms->flags & MAGIC_DEVICES) == 0 && sb->st_size == 0) {
			if (mime) {
				if (handle_mime(ms, mime, "x-empty") == -1)
					return -1;
			} else if (silent) {
			} else if (file_printf(ms, "%sempty", COMMA) == -1)
				return -1;
			break;
		}
		ret = 0;
		break;

	default:
		file_error(ms, 0, "invalid mode 0%o", sb->st_mode);
		return -1;
		/*NOTREACHED*/
	}

	if (!silent && !mime && did && ret == 0) {
	    if (file_printf(ms, " ") == -1)
		    return -1;
	}
	/*
	 * If we were looking for extensions or apple (silent) it is not our
	 * job to print here, so don't count this as a match.
	 */
	if (ret == 1 && silent)
		return 0;
	return ret;
}
