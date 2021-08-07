
#include "file.h"

#include "magic.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <limits.h>	/* for PIPE_BUF */

#include <sys/time.h>
#include <utime.h>

#include <unistd.h>	/* for read() */

private void close_and_restore(const struct magic_set *, const char *, int,
    const struct stat *);
private int unreadable_info(struct magic_set *, mode_t, const char *);
private const char* get_default_magic(void);
private const char *file_or_fd(struct magic_set *, const char *, int);

#define	STDIN_FILENO	0

private const char *
get_default_magic(void)
{
	static const char hmagic[] = "/.magic/magic.mgc";
	static char *default_magic;
	char *home, *hmagicpath;

	struct stat st;

	if (default_magic) {
		free(default_magic);
		default_magic = NULL;
	}
	if ((home = getenv("HOME")) == NULL)
		return MAGIC;

	if (asprintf(&hmagicpath, "%s/.magic.mgc", home) < 0)
		return MAGIC;
	if (stat(hmagicpath, &st) == -1) {
		free(hmagicpath);
		if (asprintf(&hmagicpath, "%s/.magic", home) < 0)
			return MAGIC;
		if (stat(hmagicpath, &st) == -1)
			goto out;
		if (S_ISDIR(st.st_mode)) {
			free(hmagicpath);
			if (asprintf(&hmagicpath, "%s/%s", home, hmagic) < 0)
				return MAGIC;
			if (access(hmagicpath, R_OK) == -1)
				goto out;
		}
	}

	if (asprintf(&default_magic, "%s:%s", hmagicpath, MAGIC) < 0)
		goto out;
	free(hmagicpath);
	return default_magic;
out:
	default_magic = NULL;
	free(hmagicpath);
	return MAGIC;
}

public const char *
magic_getpath(const char *magicfile, int action)
{
	if (magicfile != NULL)
		return magicfile;

	magicfile = getenv("MAGIC");
	if (magicfile != NULL)
		return magicfile;

	return action == FILE_LOAD ? get_default_magic() : MAGIC;
}

public struct magic_set *
magic_open(int flags)
{
	return file_ms_alloc(flags);
}

private int
unreadable_info(struct magic_set *ms, mode_t md, const char *file)
{
	if (file) {
		/* We cannot open it, but we were able to stat it. */
		if (access(file, W_OK) == 0)
			if (file_printf(ms, "writable, ") == -1)
				return -1;
		if (access(file, X_OK) == 0)
			if (file_printf(ms, "executable, ") == -1)
				return -1;
	}
	if (S_ISREG(md))
		if (file_printf(ms, "regular file, ") == -1)
			return -1;
	if (file_printf(ms, "no read permission") == -1)
		return -1;
	return 0;
}

public void
magic_close(struct magic_set *ms)
{
	if (ms == NULL)
		return;
	file_ms_free(ms);
}

/*
 * load a magic file
 */
public int
magic_load(struct magic_set *ms, const char *magicfile)
{
	if (ms == NULL)
		return -1;
	return file_apprentice(ms, magicfile, FILE_LOAD);
}

/*
 * Install a set of compiled magic buffers.
 */
public int
magic_load_buffers(struct magic_set *ms, void **bufs, size_t *sizes,
    size_t nbufs)
{
	if (ms == NULL)
		return -1;
	return buffer_apprentice(ms, RCAST(struct magic **, bufs),
	    sizes, nbufs);
}

public int
magic_compile(struct magic_set *ms, const char *magicfile)
{
	if (ms == NULL)
		return -1;
	return file_apprentice(ms, magicfile, FILE_COMPILE);
}

public int
magic_check(struct magic_set *ms, const char *magicfile)
{
	if (ms == NULL)
		return -1;
	return file_apprentice(ms, magicfile, FILE_CHECK);
}

public int
magic_list(struct magic_set *ms, const char *magicfile)
{
	if (ms == NULL)
		return -1;
	return file_apprentice(ms, magicfile, FILE_LIST);
}

private void
close_and_restore(const struct magic_set *ms, const char *name, int fd,
    const struct stat *sb)
{
	if (fd == STDIN_FILENO || name == NULL)
		return;
	(void) close(fd);

	if ((ms->flags & MAGIC_PRESERVE_ATIME) != 0) {
		/*
		 * Try to restore access, modification times if read it.
		 * This is really *bad* because it will modify the status
		 * time of the file... And of course this will affect
		 * backup programs
		 */

		struct timeval  utsbuf[2];
		(void)memset(utsbuf, 0, sizeof(utsbuf));
		utsbuf[0].tv_sec = sb->st_atime;
		utsbuf[1].tv_sec = sb->st_mtime;

		(void) utimes(name, utsbuf); /* don't care if loses */
	}
}

/*
 * find type of descriptor
 */
public const char *
magic_descriptor(struct magic_set *ms, int fd)
{
	if (ms == NULL)
		return NULL;
	return file_or_fd(ms, NULL, fd);
}

/*
 * find type of named file
 */
public const char *
magic_file(struct magic_set *ms, const char *inname)
{
	if (ms == NULL)
		return NULL;
	return file_or_fd(ms, inname, STDIN_FILENO);
}

private const char *
file_or_fd(struct magic_set *ms, const char *inname, int fd)
{
	int	rv = -1;
	unsigned char *buf;
	struct stat	sb;
	ssize_t nbytes = 0;	/* number of bytes read from a datafile */
	int	ispipe = 0;
	int	okstat = 0;
	off_t	pos = CAST(off_t, -1);

	if (file_reset(ms, 1) == -1)
		goto out;

	/*
	 * one extra for terminating '\0', and
	 * some overlapping space for matches near EOF
	 */
#define SLOP (1 + sizeof(union VALUETYPE))
	if ((buf = CAST(unsigned char *, malloc(ms->bytes_max + SLOP))) == NULL)
		return NULL;

	switch (file_fsmagic(ms, inname, &sb)) {
	case -1:		/* error */
		goto done;
	case 0:			/* nothing found */
		break;
	default:		/* matched it and printed type */
		rv = 0;
		goto done;
	}

	if (inname != NULL) {
		int flags = O_RDONLY|O_BINARY|O_NONBLOCK|O_CLOEXEC;
		errno = 0;
		if ((fd = open(inname, flags)) < 0) {
			okstat = stat(inname, &sb) == 0;
			if (okstat && S_ISFIFO(sb.st_mode))
				ispipe = 1;
			if (okstat &&
			    unreadable_info(ms, sb.st_mode, inname) == -1)
				goto done;
			rv = 0;
			goto done;
		}

	}

	if (fd != -1) {
		okstat = fstat(fd, &sb) == 0;
		if (okstat && S_ISFIFO(sb.st_mode))
			ispipe = 1;
		if (inname == NULL)
			pos = lseek(fd, CAST(off_t, 0), SEEK_CUR);
	}

	/*
	 * try looking at the first ms->bytes_max bytes
	 */
	if (ispipe) {
		if (fd != -1) {
			ssize_t r = 0;

			while ((r = sread(fd, RCAST(void *, &buf[nbytes]),
			    CAST(size_t, ms->bytes_max - nbytes), 1)) > 0) {
				nbytes += r;
				if (r < PIPE_BUF) break;
			}
		}

		if (nbytes == 0 && inname) {
			/* We can not read it, but we were able to stat it. */
			if (unreadable_info(ms, sb.st_mode, inname) == -1)
				goto done;
			rv = 0;
			goto done;
		}

	} else if (fd != -1) {
		/* Windows refuses to read from a big console buffer. */
		size_t howmany =ms->bytes_max;
		if ((nbytes = read(fd, RCAST(void *, buf), howmany)) == -1) {
			if (inname == NULL && fd != STDIN_FILENO)
				file_error(ms, errno, "cannot read fd %d", fd);
			else
				file_error(ms, errno, "cannot read `%s'",
				    inname == NULL ? "/dev/stdin" : inname);
			goto done;
		}
	}

	(void)memset(buf + nbytes, 0, SLOP); /* NUL terminate */
	if (file_buffer(ms, fd, okstat ? &sb : NULL, inname, buf, CAST(size_t, nbytes)) == -1)
		goto done;
	rv = 0;
done:
	free(buf);
	if (fd != -1) {
		if (pos != CAST(off_t, -1))
			(void)lseek(fd, pos, SEEK_SET);
		close_and_restore(ms, inname, fd, &sb);
	}
out:
	return rv == 0 ? file_getbuffer(ms) : NULL;
}


public const char *
magic_buffer(struct magic_set *ms, const void *buf, size_t nb)
{
	if (ms == NULL)
		return NULL;
	if (file_reset(ms, 1) == -1)
		return NULL;
	/*
	 * The main work is done here!
	 * We have the file name and/or the data buffer to be identified.
	 */
	if (file_buffer(ms, -1, NULL, NULL, buf, nb) == -1) {
		return NULL;
	}
	return file_getbuffer(ms);
}

public const char *
magic_error(struct magic_set *ms)
{
	if (ms == NULL)
		return "Magic database is not open";
	return (ms->event_flags & EVENT_HAD_ERR) ? ms->o.buf : NULL;
}

public int
magic_errno(struct magic_set *ms)
{
	if (ms == NULL)
		return EINVAL;
	return (ms->event_flags & EVENT_HAD_ERR) ? ms->error : 0;
}

public int
magic_getflags(struct magic_set *ms)
{
	if (ms == NULL)
		return -1;

	return ms->flags;
}

public int
magic_setflags(struct magic_set *ms, int flags)
{
	if (ms == NULL)
		return -1;

	ms->flags = flags;
	return 0;
}

public int
magic_version(void)
{
	return MAGIC_VERSION;
}

public int
magic_setparam(struct magic_set *ms, int param, const void *val)
{
	if (ms == NULL)
		return -1;
	switch (param) {
	case MAGIC_PARAM_INDIR_MAX:
		ms->indir_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_NAME_MAX:
		ms->name_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_ELF_PHNUM_MAX:
		ms->elf_phnum_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_ELF_SHNUM_MAX:
		ms->elf_shnum_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_ELF_NOTES_MAX:
		ms->elf_notes_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_REGEX_MAX:
		ms->regex_max = CAST(uint16_t, *CAST(const size_t *, val));
		return 0;
	case MAGIC_PARAM_BYTES_MAX:
		ms->bytes_max = *CAST(const size_t *, val);
		return 0;
	case MAGIC_PARAM_ENCODING_MAX:
		ms->encoding_max = *CAST(const size_t *, val);
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}

public int
magic_getparam(struct magic_set *ms, int param, void *val)
{
	if (ms == NULL)
		return -1;
	switch (param) {
	case MAGIC_PARAM_INDIR_MAX:
		*CAST(size_t *, val) = ms->indir_max;
		return 0;
	case MAGIC_PARAM_NAME_MAX:
		*CAST(size_t *, val) = ms->name_max;
		return 0;
	case MAGIC_PARAM_ELF_PHNUM_MAX:
		*CAST(size_t *, val) = ms->elf_phnum_max;
		return 0;
	case MAGIC_PARAM_ELF_SHNUM_MAX:
		*CAST(size_t *, val) = ms->elf_shnum_max;
		return 0;
	case MAGIC_PARAM_ELF_NOTES_MAX:
		*CAST(size_t *, val) = ms->elf_notes_max;
		return 0;
	case MAGIC_PARAM_REGEX_MAX:
		*CAST(size_t *, val) = ms->regex_max;
		return 0;
	case MAGIC_PARAM_BYTES_MAX:
		*CAST(size_t *, val) = ms->bytes_max;
		return 0;
	case MAGIC_PARAM_ENCODING_MAX:
		*CAST(size_t *, val) = ms->encoding_max;
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}
