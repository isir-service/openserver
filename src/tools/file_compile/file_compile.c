#define _GNU_SOURCE
#include <stdio.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "base/oplog.h"

#include "file_compile.h"

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */
const char ext[] = ".mgc";
const size_t file_nnames = FILE_NAMES_SIZE;

char text_chars[256] = {
	/*                  BEL BS HT LF VT FF CR    */
	F, F, F, F, F, F, F, T, T, T, T, T, T, T, F, F,  /* 0x0X */
	/*                              ESC          */
	F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
	T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
	/*            NEL                            */
	X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
	X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
	I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};


#define	XX 0xF1 // invalid: size 1
#define	AS 0xF0 // ASCII: size 1
#define	S1 0x02 // accept 0, size 2
#define	S2 0x13 // accept 1, size 3
#define	S3 0x03 // accept 0, size 3
#define	S4 0x23 // accept 2, size 3
#define	S5 0x34 // accept 3, size 4
#define	S6 0x04 // accept 0, size 4
#define	S7 0x44 // accept 4, size 4

#define LOCB 0x80
#define HICB 0xBF

// first is information about the first byte in a UTF-8 sequence.
static const uint8_t first[] = {
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x00-0x0F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x10-0x1F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x20-0x2F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x30-0x3F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x40-0x4F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x50-0x5F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x60-0x6F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x70-0x7F
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0x80-0x8F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0x90-0x9F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xA0-0xAF
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xB0-0xBF
    XX, XX, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, // 0xC0-0xCF
    S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, // 0xD0-0xDF
    S2, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S4, S3, S3, // 0xE0-0xEF
    S5, S6, S6, S6, S7, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xF0-0xFF
};

// acceptRange gives the range of valid values for the second byte in a UTF-8
// sequence.
struct accept_range {
	uint8_t lo; // lowest value for second byte.
	uint8_t hi; // highest value for second byte.
};

struct accept_range accept_ranges[16] = {
// acceptRanges has size 16 to avoid bounds checks in the code that uses it.
	{ LOCB, HICB },
	{ 0xA0, HICB },
	{ LOCB, 0x9F },
	{ 0x90, HICB },
	{ LOCB, 0x8F },
};


size_t magicsize = sizeof(struct magic);
#define	DECLARE_FIELD(name) { # name, sizeof(# name) - 1, parse_ ## name }
#define	EATAB {while (isascii(CAST(unsigned char, *l)) && isspace(CAST(unsigned char, *l)))  ++l;}
#define LOWCASE(l) (isupper(CAST(unsigned char, l)) ? tolower(CAST(unsigned char, l)) : (l))

#define SIZE_T_FORMAT "z"
#define INT64_T_FORMAT "ll"
#define INTMAX_T_FORMAT "j"
#define ALLOC_CHUNK	CAST(size_t, 10)
#define ALLOC_INCR	CAST(size_t, 200)

static const struct type_tbl_s special_tbl[] = {
	{ XXD("der"),		FILE_DER,		FILE_FMT_STR },
	{ XXD("name"),		FILE_NAME,		FILE_FMT_STR },
	{ XXD("use"),		FILE_USE,		FILE_FMT_STR },
	{ XXD_NULL,		FILE_INVALID,		FILE_FMT_NONE },
};

const size_t file_nformats = FILE_NAMES_SIZE;

size_t typesize(int type)
{
	switch (type) {
	case FILE_BYTE:
		return 1;

	case FILE_SHORT:
	case FILE_LESHORT:
	case FILE_BESHORT:
		return 2;

	case FILE_LONG:
	case FILE_LELONG:
	case FILE_BELONG:
	case FILE_MELONG:
		return 4;

	case FILE_DATE:
	case FILE_LEDATE:
	case FILE_BEDATE:
	case FILE_MEDATE:
	case FILE_LDATE:
	case FILE_LELDATE:
	case FILE_BELDATE:
	case FILE_MELDATE:
	case FILE_FLOAT:
	case FILE_BEFLOAT:
	case FILE_LEFLOAT:
		return 4;

	case FILE_QUAD:
	case FILE_BEQUAD:
	case FILE_LEQUAD:
	case FILE_QDATE:
	case FILE_LEQDATE:
	case FILE_BEQDATE:
	case FILE_QLDATE:
	case FILE_LEQLDATE:
	case FILE_BEQLDATE:
	case FILE_QWDATE:
	case FILE_LEQWDATE:
	case FILE_BEQWDATE:
	case FILE_DOUBLE:
	case FILE_BEDOUBLE:
	case FILE_LEDOUBLE:
	case FILE_OFFSET:
		return 8;

	case FILE_GUID:
		return 16;

	default:
		return FILE_BADSIZE;
	}
}

int goodchar(unsigned char x, const char *extra)
{
	return (isascii(x) && isalnum(x)) || strchr(extra, x);
}

int parse_extra(struct magic_set *ms, struct magic_entry *me, const char *line,
		size_t llen, off_t off, size_t len, const char *name, const char *extra,
		int nt)
{
	size_t i;
	const char *l = line;
	struct magic *m = &me->mp[me->cont_count == 0 ? 0 : me->cont_count - 1];
	char *buf = CAST(char *, CAST(void *, m)) + off;

	if (buf[0] != '\0') {
		len = nt ? strlen(buf) : len;
		log_warn_ex("Current entry already has a %s type %.*s, new type %s\n", name, buf, l);
		return -1;
	}

	if (*m->desc == '\0') {
		log_warn_ex("Current entry does not yet have a description for adding a %s type\n", name);
		return -1;
	}

	EATAB;
	for (i = 0; *l && i < llen && i < len && goodchar(*l, extra);buf[i++] = *l++)
		continue;

	if (i == len && *l) {
		if (nt)
			buf[len - 1] = '\0';
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("%s type %s truncated %" SIZE_T_FORMAT "u\n", name, line, i);
	} else {
		if (!isspace(CAST(unsigned char, *l)) && !goodchar(*l, extra))
			log_warn_ex("%s type %s has bad char %c\n",name, line, *l);
		if (nt)
			buf[i] = '\0';
	}

	if (i > 0)
		return 0;

	log_warn_ex("Bad magic entry %s\n", line);
	return -1;
}


int parse_apple(struct magic_set *ms, struct magic_entry *me, const char *line,size_t len)
{
	struct magic *m = &me->mp[0];

	return parse_extra(ms, me, line, len,
		CAST(off_t, offsetof(struct magic, apple)),
		sizeof(m->apple), "APPLE", "!+-./?", 0);
}

int parse_ext(struct magic_set *ms, struct magic_entry *me, const char *line,size_t len)
{
	struct magic *m = &me->mp[0];

	return parse_extra(ms, me, line, len,
		CAST(off_t, offsetof(struct magic, ext)),
		sizeof(m->ext), "EXTENSION", ",!+-/@?_$&", 0);
}

int parse_mime(struct magic_set *ms, struct magic_entry *me, const char *line,size_t len)
{
	struct magic *m = &me->mp[0];

	return parse_extra(ms, me, line, len,
		CAST(off_t, offsetof(struct magic, mimetype)),
		sizeof(m->mimetype), "MIME", "+-/.$?:{}", 1);
}

int parse_strength(struct magic_set *ms, struct magic_entry *me, const char *line,size_t len __attribute__((__unused__)))
{
	const char *l = line;
	char *el;
	unsigned long factor;
	struct magic *m = &me->mp[0];

	if (m->factor_op != FILE_FACTOR_OP_NONE) {
		log_warn_ex("Current entry already has a strength type: %c %d\n",m->factor_op, m->factor);
		return -1;
	}
	if (m->type == FILE_NAME) {
		log_warn_ex("%s: Strength setting is not supported in \"name\" magic entries", m->value.s);
		return -1;
	}
	EATAB;
	switch (*l) {
	case FILE_FACTOR_OP_NONE:
	case FILE_FACTOR_OP_PLUS:
	case FILE_FACTOR_OP_MINUS:
	case FILE_FACTOR_OP_TIMES:
	case FILE_FACTOR_OP_DIV:
		m->factor_op = *l++;
		break;
	default:
		log_warn_ex("Unknown factor op `%c'\n", *l);
		return -1;
	}
	EATAB;
	factor = strtoul(l, &el, 0);
	if (factor > 255) {
		log_warn_ex("Too large factor `%lu'\n", factor);
		goto out;
	}
	if (*el && !isspace(CAST(unsigned char, *el))) {
		log_warn_ex("Bad factor `%s'\n", l);
		goto out;
	}
	m->factor = CAST(uint8_t, factor);
	if (m->factor == 0 && m->factor_op == FILE_FACTOR_OP_DIV) {
		log_warn_ex("Cannot have factor op `%c' and factor %u",m->factor_op, m->factor);
		goto out;
	}
	return 0;
out:
	m->factor_op = FILE_FACTOR_OP_NONE;
	m->factor = 0;
	return -1;
}


struct s_bang{
	const char *name;
	size_t len;
	int (*fun)(struct magic_set *, struct magic_entry *, const char *,size_t);
};

struct s_bang bang[] = {
	{"mime", sizeof("name") - 1, parse_mime,},
	{"apple", sizeof("apple") - 1, parse_apple},
	{"ext" ,sizeof("ext") - 1, parse_ext},
	{"strength", sizeof("strength") - 1, parse_strength},
	{ NULL, 0, NULL }
};

int magic_setflags(struct magic_set *ms, int flags)
{
	if (!ms)
		return -1;

	ms->flags = flags;
	return 0;
}

struct magic_set *file_ms_alloc(int flags)
{
	struct magic_set *ms;
	size_t i, len;

	ms = calloc(1, sizeof(struct magic_set));
	if (!ms) {
		log_warn_ex("calloc failed\n");
		return NULL;
	}

	if (magic_setflags(ms, flags) == -1) {
		log_warn_ex("magic_setflags failed\n");
		goto _free;
	}

	ms->o.buf = ms->o.pbuf = NULL;
	ms->o.blen = 0;
	len = (ms->c.len = 10) * sizeof(*ms->c.li);

	ms->c.li = calloc(1, len);

	if (!ms->c.li) {
		log_warn_ex("calloc failed\n");
		goto _free;
	}

	ms->event_flags = 0;
	ms->error = -1;
	for (i = 0; i < MAGIC_SETS; i++)
		ms->mlist[i] = NULL;
	ms->file = "unknown";
	ms->line = 0;
	ms->indir_max = FILE_INDIR_MAX;
	ms->name_max = FILE_NAME_MAX;
	ms->elf_shnum_max = FILE_ELF_SHNUM_MAX;
	ms->elf_phnum_max = FILE_ELF_PHNUM_MAX;
	ms->elf_notes_max = FILE_ELF_NOTES_MAX;
	ms->regex_max = FILE_REGEX_MAX;
	ms->bytes_max = FILE_BYTES_MAX;
	ms->encoding_max = FILE_ENCODING_MAX;
	return ms;
_free:
	free(ms);
	return NULL;
}

void file_clearbuf(struct magic_set *ms)
{
	if (ms->o.buf)
		free(ms->o.buf);
	ms->o.buf = NULL;
	ms->o.blen = 0;
	return;
}

int file_reset(struct magic_set *ms, int checkloaded)
{
	if (checkloaded && ms->mlist[0] == NULL) {
		log_warn_ex("no magic files loaded\n");
		return -1;
	}

	file_clearbuf(ms);
	if (ms->o.pbuf) {
		free(ms->o.pbuf);
		ms->o.pbuf = NULL;
	}
	ms->event_flags &= ~EVENT_HAD_ERR;
	ms->error = -1;
	return 0;
}

void init_file_tables(void)
{
	static int done = 0;
	const struct type_tbl_s *p;

	if (done)
		return;
	done++;

	for (p = type_tbl; p->len; p++) {
		if(p->type >= FILE_NAMES_SIZE) {
			log_error_ex("type is too large, type= %d\n",p->type);
			return;
		}
		file_names[p->type] = p->name;
		file_formats[p->type] = p->format;
	}
	
	if(p - type_tbl != FILE_NAMES_SIZE) {
		log_error_ex("FILE_NAMES_SIZE is not equal\n");
		return;
	}

	return;
}

void apprentice_unmap(struct magic_map *map)
{
	size_t i;
	if (!map)
		return;

	switch (map->type) {
		case MAP_TYPE_USER:
			break;
		case MAP_TYPE_MALLOC:
			for (i = 0; i < MAGIC_SETS; i++) {
				void *b = map->magic[i];
				void *p = map->p;
				if (CAST(char *, b) >= CAST(char *, p) &&
					CAST(char *, b) <= CAST(char *, p) + map->len)
					continue;
				free(map->magic[i]);
			}
			free(map->p);
			break;
#ifdef QUICK
		case MAP_TYPE_MMAP:
			if (map->p && map->p != MAP_FAILED)
				(void)munmap(map->p, map->len);
			break;
#endif
		default:
			log_error_ex("map->type failed, type= %d\n",map->type);
	}
	free(map);
}

void mlist_free_one(struct mlist *ml)
{
	if (ml->map)
		apprentice_unmap(CAST(struct magic_map *, ml->map));
	free(ml);
}

void mlist_free(struct mlist *mlist)
{
	struct mlist *ml, *next;

	if (mlist == NULL)
		return;

	for (ml = mlist->next; ml != mlist;) {
		next = ml->next;
		mlist_free_one(ml);
		ml = next;
	}

	mlist_free_one(mlist);

	return;
}

struct mlist *mlist_alloc(void)
{
	struct mlist *mlist;
	if ((mlist = CAST(struct mlist *, calloc(1, sizeof(*mlist)))) == NULL) {
		return NULL;
	}
	mlist->next = mlist->prev = mlist;
	return mlist;
}

int cmpstrp(const void *p1, const void *p2)
{
	return strcmp(*RCAST(char *const *, p1), *RCAST(char *const *, p2));
}

int file_check_mem(struct magic_set *ms, unsigned int level)
{
	size_t len;

	if (level >= ms->c.len) {
		len = (ms->c.len = 20 + level) * sizeof(*ms->c.li);
		ms->c.li = CAST(struct level_info *, (ms->c.li == NULL) ?
				malloc(len) :
				realloc(ms->c.li, len));
		if (ms->c.li == NULL) {
			log_warn_ex("file_check_mem failed\n");
			return -1;
		}
	}
	ms->c.li[level].got_match = 0;

	ms->c.li[level].last_match = 0;
	ms->c.li[level].last_cond = COND_NONE;
	return 0;
}

int get_op(char c)
{
	switch (c) {
	case '&':
		return FILE_OPAND;
	case '|':
		return FILE_OPOR;
	case '^':
		return FILE_OPXOR;
	case '+':
		return FILE_OPADD;
	case '-':
		return FILE_OPMINUS;
	case '*':
		return FILE_OPMULTIPLY;
	case '/':
		return FILE_OPDIVIDE;
	case '%':
		return FILE_OPMODULO;
	default:
		return -1;
	}
}

int get_cond(const char *l, const char **t)
{
	static const struct cond_tbl_s {
		char name[8];
		size_t len;
		int cond;
	} cond_tbl[] = {
		{ "if",		2,	COND_IF },
		{ "elif",	4,	COND_ELIF },
		{ "else",	4,	COND_ELSE },
		{ "",		0,	COND_NONE },
	};
	const struct cond_tbl_s *p;

	for (p = cond_tbl; p->len; p++) {
		if (strncmp(l, p->name, p->len) == 0 &&
		    isspace(CAST(unsigned char, l[p->len]))) {
			if (t)
				*t = l + p->len;
			break;
		}
	}
	return p->cond;
}

int check_cond(struct magic_set *ms, int cond, uint32_t cont_level)
{
	int last_cond;
	last_cond = ms->c.li[cont_level].last_cond;

	switch (cond) {
	case COND_IF:
		if (last_cond != COND_NONE && last_cond != COND_ELIF) {
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("syntax error: `if' \n");
			return -1;
		}
		last_cond = COND_IF;
		break;

	case COND_ELIF:
		if (last_cond != COND_IF && last_cond != COND_ELIF) {
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("syntax error: `elif'\n");
			return -1;
		}
		last_cond = COND_ELIF;
		break;

	case COND_ELSE:
		if (last_cond != COND_IF && last_cond != COND_ELIF) {
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("syntax error: `else'\n");
			return -1;
		}
		last_cond = COND_NONE;
		break;

	case COND_NONE:
		last_cond = COND_NONE;
		break;
	}

	ms->c.li[cont_level].last_cond = last_cond;
	return 0;
}

int get_type(const struct type_tbl_s *tbl, const char *l, const char **t)
{
	const struct type_tbl_s *p;

	for (p = tbl; p->len; p++) {
		if (strncmp(l, p->name, p->len) == 0) {
			if (t)
				*t = l + p->len;
			break;
		}
	}
	return p->type;
}

int get_standard_integer_type(const char *l, const char **t)
{
	int type;

	if (isalpha(CAST(unsigned char, l[1]))) {
		switch (l[1]) {
			case 'C':
				/* "dC" and "uC" */
				type = FILE_BYTE;
				break;
			case 'S':
				/* "dS" and "uS" */
				type = FILE_SHORT;
				break;
			case 'I':
			case 'L':
				/*
				 * "dI", "dL", "uI", and "uL".
				 *
				 * XXX - the actual Single UNIX Specification says
				 * that "L" means "long", as in the C data type,
				 * but we treat it as meaning "4-byte integer".
				 * Given that the OS X version of file 5.04 did
				 * the same, I guess that passes the actual SUS
				 * validation suite; having "dL" be dependent on
				 * how big a "long" is on the machine running
				 * "file" is silly.
				 */
				type = FILE_LONG;
				break;
			case 'Q':
				/* "dQ" and "uQ" */
				type = FILE_QUAD;
				break;
			default:
				/* "d{anything else}", "u{anything else}" */
				return FILE_INVALID;
		}
		l += 2;
	} else if (isdigit(CAST(unsigned char, l[1]))) {
		/*
		 * "d{num}" and "u{num}"; we only support {num} values
		 * of 1, 2, 4, and 8 - the Single UNIX Specification
		 * doesn't say anything about whether arbitrary
		 * values should be supported, but both the Solaris 10
		 * and OS X Mountain Lion versions of file passed the
		 * Single UNIX Specification validation suite, and
		 * neither of them support values bigger than 8 or
		 * non-power-of-2 values.
		 */
		if (isdigit(CAST(unsigned char, l[2]))) {
			/* Multi-digit, so > 9 */
			return FILE_INVALID;
		}
		switch (l[1]) {
			case '1':
				type = FILE_BYTE;
				break;
			case '2':
				type = FILE_SHORT;
				break;
			case '4':
				type = FILE_LONG;
				break;
			case '8':
				type = FILE_QUAD;
				break;
			default:
				/* XXX - what about 3, 5, 6, or 7? */
				return FILE_INVALID;
		}
		l += 2;
	} else {
		/*
		 * "d" or "u" by itself.
		 */
		type = FILE_LONG;
		++l;
	}
	if (t)
		*t = l;
	return type;
}

int parse_indirect_modifier(struct magic_set *ms, struct magic *m, const char **lp)
{
	const char *l = *lp;

	while (!isspace(CAST(unsigned char, *++l)))
		switch (*l) {
		case CHAR_INDIRECT_RELATIVE:
			m->str_flags |= INDIRECT_RELATIVE;
			break;
		default:
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("indirect modifier `%c' invalid\n", *l);
			*lp = l;
			return -1;
		}
	*lp = l;
	return 0;
}

int string_modifier_check(struct magic_set *ms, struct magic *m)
{
	if ((ms->flags & MAGIC_CHECK) == 0)
		return 0;

	if ((m->type != FILE_REGEX || (m->_u._s._flags & REGEX_LINE_COUNT) == 0) &&
	    (m->type != FILE_PSTRING && (m->_u._s._flags & PSTRING_LEN) != 0)) {
		log_warn_ex("'/BHhLl' modifiers are only allowed for pascal strings\n");
		return -1;
	}
	switch (m->type) {
	case FILE_BESTRING16:
	case FILE_LESTRING16:
		if (m->_u._s._flags != 0) {
			log_warn_ex("no modifiers allowed for 16-bit strings\n");
			return -1;
		}
		break;
	case FILE_STRING:
	case FILE_PSTRING:
		if ((m->_u._s._flags & REGEX_OFFSET_START) != 0) {
			log_warn_ex("'/%c' only allowed on regex and search\n",CHAR_REGEX_OFFSET_START);
			return -1;
		}
		break;
	case FILE_SEARCH:
		if (m->_u._s._count == 0) {
			log_warn_ex("missing range; defaulting to %d\n",STRING_DEFAULT_RANGE);
			m->_u._s._count = STRING_DEFAULT_RANGE;
			return -1;
		}
		break;
	case FILE_REGEX:
		if ((m->_u._s._flags & STRING_COMPACT_WHITESPACE) != 0) {
			log_warn_ex("'/%c' not allowed on regex\n",CHAR_COMPACT_WHITESPACE);
			return -1;
		}
		if ((m->_u._s._flags & STRING_COMPACT_OPTIONAL_WHITESPACE) != 0) {
			log_warn_ex("'/%c' not allowed on regex\n",CHAR_COMPACT_OPTIONAL_WHITESPACE);
			return -1;
		}
		break;
	default:
		log_warn_ex("coding error: m->type=%d\n", m->type);
		return -1;
	}
	return 0;
}

int parse_string_modifier(struct magic_set *ms, struct magic *m, const char **lp)
{
	const char *l = *lp;
	char *t;
	int have_range = 0;

	while (!isspace(CAST(unsigned char, *++l))) {
		switch (*l) {
		case '0':  case '1':  case '2':
		case '3':  case '4':  case '5':
		case '6':  case '7':  case '8':
		case '9':
			if (have_range && (ms->flags & MAGIC_CHECK))
				log_warn_ex("multiple ranges\n");
			have_range = 1;
			m->_u._s._count = CAST(uint32_t, strtoul(l, &t, 0));
			if (m->_u._s._count == 0)
				log_warn_ex("zero range\n");
			l = t - 1;
			break;
		case CHAR_COMPACT_WHITESPACE:
			m->_u._s._flags |= STRING_COMPACT_WHITESPACE;
			break;
		case CHAR_COMPACT_OPTIONAL_WHITESPACE:
			m->_u._s._flags |= STRING_COMPACT_OPTIONAL_WHITESPACE;
			break;
		case CHAR_IGNORE_LOWERCASE:
			m->_u._s._flags |= STRING_IGNORE_LOWERCASE;
			break;
		case CHAR_IGNORE_UPPERCASE:
			m->_u._s._flags |= STRING_IGNORE_UPPERCASE;
			break;
		case CHAR_REGEX_OFFSET_START:
			m->_u._s._flags |= REGEX_OFFSET_START;
			break;
		case CHAR_BINTEST:
			m->_u._s._flags |= STRING_BINTEST;
			break;
		case CHAR_TEXTTEST:
			m->_u._s._flags |= STRING_TEXTTEST;
			break;
		case CHAR_TRIM:
			m->_u._s._flags |= STRING_TRIM;
			break;
		case CHAR_PSTRING_1_LE:
#define SET_LENGTH(a) m->_u._s._flags = (m->_u._s._flags & ~PSTRING_LEN) | (a)
			if (m->type != FILE_PSTRING)
				goto bad;
			SET_LENGTH(PSTRING_1_LE);
			break;
		case CHAR_PSTRING_2_BE:
			if (m->type != FILE_PSTRING)
				goto bad;
			SET_LENGTH(PSTRING_2_BE);
			break;
		case CHAR_PSTRING_2_LE:
			if (m->type != FILE_PSTRING)
				goto bad;
			SET_LENGTH(PSTRING_2_LE);
			break;
		case CHAR_PSTRING_4_BE:
			if (m->type != FILE_PSTRING)
				goto bad;
			SET_LENGTH(PSTRING_4_BE);
			break;
		case CHAR_PSTRING_4_LE:
			switch (m->type) {
			case FILE_PSTRING:
			case FILE_REGEX:
				break;
			default:
				goto bad;
			}
			SET_LENGTH(PSTRING_4_LE);
			break;
		case CHAR_PSTRING_LENGTH_INCLUDES_ITSELF:
			if (m->type != FILE_PSTRING)
				goto bad;
			m->_u._s._flags |= PSTRING_LENGTH_INCLUDES_ITSELF;
			break;
		default:
		bad:
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("string modifier `%c' invalid", *l);
			goto out;
		}
		/* allow multiple '/' for readability */
		if (l[1] == '/' && !isspace(CAST(unsigned char, l[2])))
			l++;
	}
	if (string_modifier_check(ms, m) == -1)
		goto out;
	*lp = l;
	return 0;
out:
	*lp = l;
	return -1;
}

uint64_t file_signextend(struct magic_set *ms, struct magic *m, uint64_t v)
{
	if (!(m->flag & UNSIGNED)) {
		switch(m->type) {
		/*
		 * Do not remove the casts below.  They are
		 * vital.  When later compared with the data,
		 * the sign extension must have happened.
		 */
		case FILE_BYTE:
			v = CAST(signed char,  v);
			break;
		case FILE_SHORT:
		case FILE_BESHORT:
		case FILE_LESHORT:
			v = CAST(short, v);
			break;
		case FILE_DATE:
		case FILE_BEDATE:
		case FILE_LEDATE:
		case FILE_MEDATE:
		case FILE_LDATE:
		case FILE_BELDATE:
		case FILE_LELDATE:
		case FILE_MELDATE:
		case FILE_LONG:
		case FILE_BELONG:
		case FILE_LELONG:
		case FILE_MELONG:
		case FILE_FLOAT:
		case FILE_BEFLOAT:
		case FILE_LEFLOAT:
			v = CAST(int32_t, v);
			break;
		case FILE_QUAD:
		case FILE_BEQUAD:
		case FILE_LEQUAD:
		case FILE_QDATE:
		case FILE_QLDATE:
		case FILE_QWDATE:
		case FILE_BEQDATE:
		case FILE_BEQLDATE:
		case FILE_BEQWDATE:
		case FILE_LEQDATE:
		case FILE_LEQLDATE:
		case FILE_LEQWDATE:
		case FILE_DOUBLE:
		case FILE_BEDOUBLE:
		case FILE_LEDOUBLE:
		case FILE_OFFSET:
			v = CAST(int64_t, v);
			break;
		case FILE_STRING:
		case FILE_PSTRING:
		case FILE_BESTRING16:
		case FILE_LESTRING16:
		case FILE_REGEX:
		case FILE_SEARCH:
		case FILE_DEFAULT:
		case FILE_INDIRECT:
		case FILE_NAME:
		case FILE_USE:
		case FILE_CLEAR:
		case FILE_DER:
		case FILE_GUID:
			break;
		default:
			if (ms->flags & MAGIC_CHECK)
				log_warn("cannot happen: m->type=%d\n", m->type);
			return FILE_BADSIZE;
		}
	}
	return v;
}

void eatsize(const char **p)
{
	const char *l = *p;

	if (LOWCASE(*l) == 'u')
		l++;

	switch (LOWCASE(*l)) {
	case 'l':    /* long */
	case 's':    /* short */
	case 'h':    /* short */
	case 'b':    /* char/byte */
	case 'c':    /* char/byte */
		l++;
		/*FALLTHROUGH*/
	default:
		break;
	}

	*p = l;
}

void parse_op_modifier(struct magic_set *ms, struct magic *m, const char **lp,int op)
{
	const char *l = *lp;
	char *t;
	uint64_t val;

	++l;
	m->mask_op |= op;
	val = CAST(uint64_t, strtoull(l, &t, 0));
	l = t;
	m->_u._mask = file_signextend(ms, m, val);
	eatsize(&l);
	*lp = l;
}

int file_regcomp(file_regex_t *rx, const char *pat, int flags)
{

	rx->c_lc_ctype = newlocale(LC_CTYPE_MASK, "C", 0);
	if (!rx->c_lc_ctype) {
		log_error_ex("newlocale failed\n");
		return -1;
	}

	rx->old_lc_ctype = uselocale(rx->c_lc_ctype);

	if (!rx->old_lc_ctype) {
		log_error_ex("uselocale failed\n");
		return -1;
	}

	rx->pat = pat;

	return rx->rc = regcomp(&rx->rx, pat, flags);
}

int file_regexec(file_regex_t *rx, const char *str, size_t nmatch, regmatch_t* pmatch, int eflags)
{
	if (rx->rc) {
		log_error_ex("file_regexec failed\n");
		return -1;
	}

	/* XXX: force initialization because glibc does not always do this */
	if (nmatch != 0)
		memset(pmatch, 0, nmatch * sizeof(*pmatch));
	return regexec(&rx->rx, str, nmatch, pmatch, eflags);
}

void file_regerror(file_regex_t *rx, int rc, struct magic_set *ms)
{
	char errmsg[512];

	regerror(rc, &rx->rx, errmsg, sizeof(errmsg));
	log_warn_ex("regex error %d for `%s', (%s)", rc, rx->pat,errmsg);
	return;
}

void file_regfree(file_regex_t *rx)
{
	if (rx->rc == 0)
		regfree(&rx->rx);

	(void)uselocale(rx->old_lc_ctype);
	freelocale(rx->c_lc_ctype);
	return;
}

int file_parse_guid(const char *s, uint64_t *guid)
{
	struct guid *g = CAST(struct guid *, CAST(void *, guid));
	return sscanf(s,
		"%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
		&g->data1, &g->data2, &g->data3, &g->data4[0], &g->data4[1],
		&g->data4[2], &g->data4[3], &g->data4[4], &g->data4[5],
		&g->data4[6], &g->data4[7]) == 11 ? 0 : -1;

}

int hextoint(int c)
{
	if (!isascii(CAST(unsigned char, c)))
		return -1;
	if (isdigit(CAST(unsigned char, c)))
		return c - '0';
	if ((c >= 'a') && (c <= 'f'))
		return c + 10 - 'a';
	if (( c>= 'A') && (c <= 'F'))
		return c + 10 - 'A';
	return -1;
}

size_t file_pstring_length_size(struct magic_set *ms, const struct magic *m)
{
	switch (m->str_flags & PSTRING_LEN) {
	case PSTRING_1_LE:
		return 1;
	case PSTRING_2_LE:
	case PSTRING_2_BE:
		return 2;
	case PSTRING_4_LE:
	case PSTRING_4_BE:
		return 4;
	default:
		log_warn_ex("corrupt magic file (bad pascal string length %d)",m->_u._s._flags & PSTRING_LEN);
		return FILE_BADSIZE;
	}
}

const char *getstr(struct magic_set *ms, struct magic *m, const char *s, int warn)
{
	const char *origs = s;
	char	*p = m->value.s;
	size_t  plen = sizeof(m->value.s);
	char 	*origp = p;
	char	*pmax = p + plen - 1;
	int	c;
	int	val;

	while ((c = *s++) != '\0') {
		if (isspace(CAST(unsigned char, c)))
			break;
		if (p >= pmax) {
			log_warn_ex("string too long: `%s'\n", origs);
			return NULL;
		}
		if (c == '\\') {
			switch(c = *s++) {

			case '\0':
				if (warn)
					log_warn_ex("incomplete escape\n");
				s--;
				goto out;

			case '\t':
				if (warn) {
					log_warn_ex("escaped tab found, use \\t instead\n");
					warn = 0;	/* already did */
				}
				/*FALLTHROUGH*/
			default:
				if (warn) {
					if (isprint(CAST(unsigned char, c))) {
						/* Allow escaping of
						 * ``relations'' */
						if (strchr("<>&^=!", c) == NULL
						    && (m->type != FILE_REGEX ||
						    strchr("[]().*?^$|{}", c)
						    == NULL)) {
							log_warn_ex("no need to escape `%c'\n", c);
						}
					} else {
						log_warn_ex("unknown escape sequence: \\%03o\n", c);
					}
				}
				/*FALLTHROUGH*/
			/* space, perhaps force people to use \040? */
			case ' ':
			/* Relations */
			case '>':
			case '<':
			case '&':
			case '^':
			case '=':
			case '!':
			/* and baskslash itself */
			case '\\':
				*p++ = CAST(char, c);
				break;

			case 'a':
				*p++ = '\a';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'v':
				*p++ = '\v';
				break;

			/* \ and up to 3 octal digits */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if (c >= '0' && c <= '7') {
					val = (val << 3) | (c - '0');
					c = *s++;  /* try for 3 */
					if (c >= '0' && c <= '7')
						val = (val << 3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				*p++ = CAST(char, val);
				break;

			/* \x and up to 2 hex digits */
			case 'x':
				val = 'x';	/* Default if no digits */
				c = hextoint(*s++);	/* Get next char */
				if (c >= 0) {
					val = c;
					c = hextoint(*s++);
					if (c >= 0)
						val = (val << 4) + c;
					else
						--s;
				} else
					--s;
				*p++ = CAST(char, val);
				break;
			}
		} else
			*p++ = CAST(char, c);
	}
	--s;
out:
	*p = '\0';
	m->vallen = CAST(unsigned char, (p - origp));
	if (m->type == FILE_PSTRING) {
		size_t l =  file_pstring_length_size(ms, m);
		if (l == FILE_BADSIZE)
			return NULL;
		m->vallen += CAST(unsigned char, l);
	}
	return s;
}

int getvalue(struct magic_set *ms, struct magic *m, const char **p, int action)
{
	char *ep;
	uint64_t ull;

	switch (m->type) {
	case FILE_BESTRING16:
	case FILE_LESTRING16:
	case FILE_STRING:
	case FILE_PSTRING:
	case FILE_REGEX:
	case FILE_SEARCH:
	case FILE_NAME:
	case FILE_USE:
	case FILE_DER:
		*p = getstr(ms, m, *p, action == FILE_COMPILE);
		if (*p == NULL) {
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("cannot get string from `%s'",m->value.s);
			return -1;
		}
		if (m->type == FILE_REGEX) {
			file_regex_t rx;
			int rc = file_regcomp(&rx, m->value.s, REG_EXTENDED);
			if (rc) {
				if (ms->flags & MAGIC_CHECK)
					file_regerror(&rx, rc, ms);
			}
			file_regfree(&rx);
			return rc ? -1 : 0;
		}
		return 0;
	default:
		if (m->reln == 'x')
			return 0;
		break;
	}

	switch (m->type) {
	case FILE_FLOAT:
	case FILE_BEFLOAT:
	case FILE_LEFLOAT:
		errno = 0;

		m->value.f = (float)strtod(*p, &ep);
		if (errno == 0)
			*p = ep;
		return 0;
	case FILE_DOUBLE:
	case FILE_BEDOUBLE:
	case FILE_LEDOUBLE:
		errno = 0;
		m->value.d = strtod(*p, &ep);
		if (errno == 0)
			*p = ep;
		return 0;
	case FILE_GUID:
		if (file_parse_guid(*p, m->value.guid) == -1)
			return -1;
		*p += FILE_GUID_SIZE - 1;
		return 0;
	default:
		errno = 0;
		ull = CAST(uint64_t, strtoull(*p, &ep, 0));
		m->value.q = file_signextend(ms, m, ull);
		if (*p == ep) {
			log_warn_ex("Unparsable number `%s'", *p);
		} else {
			size_t ts = typesize(m->type);
			uint64_t x;
			const char *q;

			if (ts == FILE_BADSIZE) {
				log_warn_ex("Expected numeric type got `%s'",type_tbl[m->type].name);
			}
			for (q = *p; isspace(CAST(unsigned char, *q)); q++)
				continue;
			if (*q == '-')
				ull = -CAST(int64_t, ull);
			switch (ts) {
			case 1:
				x = CAST(uint64_t, ull & ~0xffULL);
				break;
			case 2:
				x = CAST(uint64_t, ull & ~0xffffULL);
				break;
			case 4:
				x = CAST(uint64_t, ull & ~0xffffffffULL);
				break;
			case 8:
				x = 0;
				break;
			default:
				log_error_ex("getvalue\n");;
			}
			if (x) {
				log_warn_ex("Overflow for numeric type `%s' value %llu" ,type_tbl[m->type].name, ull);
			}
		}
		if (errno == 0) {
			*p = ep;
			eatsize(p);
		}
		return 0;
	}
}

int check_format_type(const char *ptr, int type, const char **estr)
{
	int quad = 0, h;
	size_t len, cnt;
	if (*ptr == '\0') {
		/* Missing format string; bad */
		*estr = "missing format spec";
		return -1;
	}

	switch (file_formats[type]) {
	case FILE_FMT_QUAD:
		quad = 1;
		/*FALLTHROUGH*/
	case FILE_FMT_NUM:
		if (quad == 0) {
			switch (type) {
			case FILE_BYTE:
				h = 2;
				break;
			case FILE_SHORT:
			case FILE_BESHORT:
			case FILE_LESHORT:
				h = 1;
				break;
			case FILE_LONG:
			case FILE_BELONG:
			case FILE_LELONG:
			case FILE_MELONG:
			case FILE_LEID3:
			case FILE_BEID3:
			case FILE_INDIRECT:
				h = 0;
				break;
			default:
				log_error_ex("check_format_type\n");
			}
		} else
			h = 0;
		while (*ptr && strchr("-.#", *ptr) != NULL)
			ptr++;
#define CHECKLEN() do { \
	for (len = cnt = 0; isdigit(CAST(unsigned char, *ptr)); ptr++, cnt++) \
		len = len * 10 + (*ptr - '0'); \
	if (cnt > 5 || len > 1024) \
		goto toolong; \
} while (/*CONSTCOND*/0)

		CHECKLEN();
		if (*ptr == '.')
			ptr++;
		CHECKLEN();
		if (quad) {
			if (*ptr++ != 'l')
				goto invalid;
			if (*ptr++ != 'l')
				goto invalid;
		}

		switch (*ptr++) {
		case 'c':
			if (h == 2)
				return 0;
			goto invalid;
		case 'i':
		case 'd':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			return 0;
		default:
			goto invalid;
		}

	case FILE_FMT_FLOAT:
	case FILE_FMT_DOUBLE:
		if (*ptr == '-')
			ptr++;
		if (*ptr == '.')
			ptr++;
		CHECKLEN();
		if (*ptr == '.')
			ptr++;
		CHECKLEN();
		switch (*ptr++) {
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			return 0;

		default:
			goto invalid;
		}


	case FILE_FMT_STR:
		if (*ptr == '-')
			ptr++;
		while (isdigit(CAST(unsigned char, *ptr)))
			ptr++;
		if (*ptr == '.') {
			ptr++;
			while (isdigit(CAST(unsigned char , *ptr)))
				ptr++;
		}

		switch (*ptr++) {
		case 's':
			return 0;
		default:
			goto invalid;
		}

	default:
		/* internal error */
		log_error_ex("check_format_type\n");
	}
invalid:
	*estr = "not valid";
toolong:
	*estr = "too long";
	return -1;
}

int check_format(struct magic_set *ms, struct magic *m)
{
	char *ptr;
	const char *estr;

	for (ptr = m->desc; *ptr; ptr++)
		if (*ptr == '%')
			break;
	if (*ptr == '\0') {
		/* No format string; ok */
		return 1;
	}

	if(file_nformats != file_nnames) {
		log_error_ex("check_format\n");
	}

	if (m->type >= file_nformats) {
		log_warn_ex("Internal error inconsistency between m->type and format strings\n");
		return -1;
	}
	if (file_formats[m->type] == FILE_FMT_NONE) {
		log_warn_ex("No format string for `%s' with description `%s'", m->desc, file_names[m->type]);
		return -1;
	}

	ptr++;
	if (check_format_type(ptr, m->type, &estr) == -1) {
		/*
		 * TODO: this error message is unhelpful if the format
		 * string is not one character long
		 */
		log_warn_ex("Printf format is %s for type `%s' in description `%s'", estr,file_names[m->type], m->desc);
		return -1;
	}

	for (; *ptr; ptr++) {
		if (*ptr == '%') {
			log_warn_ex("Too many format strings (should have at most one) for `%s' with description `%s'",file_names[m->type], m->desc);
			return -1;
		}
	}
	return 0;
}

int parse(struct magic_set *ms, struct magic_entry *me, const char *line,size_t lineno, int action)
{
	static uint32_t last_cont_level = 0;
	size_t i;
	struct magic *m;
	const char *l = line;
	char *t;
	int op;
	uint32_t cont_level;
	int32_t diff;

	cont_level = 0;

	/*
	 * Parse the offset.
	 */
	while (*l == '>') {
		++l;		/* step over */
		cont_level++;
	}
	if (cont_level == 0 || cont_level > last_cont_level)
		if (file_check_mem(ms, cont_level) == -1)
			return -1;
	last_cont_level = cont_level;
	if (cont_level != 0) {
		if (me->mp == NULL) {
			log_warn_ex("No current entry for continuation\n");
			return -1;
		}
		if (me->cont_count == 0) {
			log_warn_ex("Continuations present with 0 count\n");
			return -1;
		}
		m = &me->mp[me->cont_count - 1];
		diff = CAST(int32_t, cont_level) - CAST(int32_t, m->cont_level);
		if (diff > 1)
			log_warn_ex("New continuation level %u is more than one larger than current level %u", cont_level,
					m->cont_level);
		if (me->cont_count == me->max_count) {
			struct magic *nm;
			size_t cnt = me->max_count + ALLOC_CHUNK;
			if ((nm = CAST(struct magic *, realloc(me->mp,sizeof(*nm) * cnt))) == NULL) {
				log_warn_ex("calloc failed\n");
				return -1;
			}
			me->mp = nm;
			me->max_count = CAST(uint32_t, cnt);
		}
		m = &me->mp[me->cont_count++];
		memset(m, 0, sizeof(*m));
		m->cont_level = cont_level;
	} else {
		static const size_t len = sizeof(*m) * ALLOC_CHUNK;
		if (me->mp != NULL)
			return 1;
		if ((m = CAST(struct magic *, malloc(len))) == NULL) {
			log_warn_ex("malloc failed\n");
			return -1;
		}
		me->mp = m;
		me->max_count = ALLOC_CHUNK;
		memset(m, 0, sizeof(*m));
		m->factor_op = FILE_FACTOR_OP_NONE;
		m->cont_level = 0;
		me->cont_count = 1;
	}
	m->lineno = CAST(uint32_t, lineno);

	if (*l == '&') {  /* m->cont_level == 0 checked below. */
                ++l;            /* step over */
                m->flag |= OFFADD;
        }
	if (*l == '(') {
		++l;		/* step over */
		m->flag |= INDIR;
		if (m->flag & OFFADD)
			m->flag = (m->flag & ~OFFADD) | INDIROFFADD;

		if (*l == '&') {  /* m->cont_level == 0 checked below */
			++l;            /* step over */
			m->flag |= OFFADD;
		}
	}
	/* Indirect offsets are not valid at level 0. */
	if (m->cont_level == 0 && (m->flag & (OFFADD | INDIROFFADD))) {
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("relative offset at level 0\n");
		return -1;
	}

	/* get offset, then skip over it */
	if (*l == '-') {
		++l;            /* step over */
		m->flag |= OFFNEGATIVE;
	}
	m->offset = CAST(int32_t, strtol(l, &t, 0));
        if (l == t) {
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("offset %s invalid\n", l);
		return -1;
	}

        l = t;

	if (m->flag & INDIR) {
		m->in_type = FILE_LONG;
		m->in_offset = 0;
		m->in_op = 0;

		if (*l == '.' || *l == ',') {
			if (*l == ',')
				m->in_op |= FILE_OPSIGNED;
			l++;
			switch (*l) {
			case 'l':
				m->in_type = FILE_LELONG;
				break;
			case 'L':
				m->in_type = FILE_BELONG;
				break;
			case 'm':
				m->in_type = FILE_MELONG;
				break;
			case 'h':
			case 's':
				m->in_type = FILE_LESHORT;
				break;
			case 'H':
			case 'S':
				m->in_type = FILE_BESHORT;
				break;
			case 'c':
			case 'b':
			case 'C':
			case 'B':
				m->in_type = FILE_BYTE;
				break;
			case 'e':
			case 'f':
			case 'g':
				m->in_type = FILE_LEDOUBLE;
				break;
			case 'E':
			case 'F':
			case 'G':
				m->in_type = FILE_BEDOUBLE;
				break;
			case 'i':
				m->in_type = FILE_LEID3;
				break;
			case 'I':
				m->in_type = FILE_BEID3;
				break;
			case 'q':
				m->in_type = FILE_LEQUAD;
				break;
			case 'Q':
				m->in_type = FILE_BEQUAD;
				break;
			default:
				if (ms->flags & MAGIC_CHECK)
					log_warn_ex("indirect offset type %c invalid",*l);
				return -1;
			}
			l++;
		}

		if (*l == '~') {
			m->in_op |= FILE_OPINVERSE;
			l++;
		}
		if ((op = get_op(*l)) != -1) {
			m->in_op |= op;
			l++;
		}
		if (*l == '(') {
			m->in_op |= FILE_OPINDIRECT;
			l++;
		}
		if (isdigit(CAST(unsigned char, *l)) || *l == '-') {
			m->in_offset = CAST(int32_t, strtol(l, &t, 0));
			if (l == t) {
				if (ms->flags & MAGIC_CHECK)
					log_warn_ex("in_offset %s invalid\n", l);
				return -1;
			}
			l = t;
		}
		if (*l++ != ')' ||
		    ((m->in_op & FILE_OPINDIRECT) && *l++ != ')')) {
			if (ms->flags & MAGIC_CHECK)
				log_warn_ex("missing ')' in indirect offset\n");
			return -1;
		}
	}
	
	EATAB;

	m->cond = get_cond(l, &l);
	if (check_cond(ms, m->cond, cont_level) == -1)
		return -1;

	EATAB;

	if (*l == 'u') {
		/*
		 * Try it as a keyword type prefixed by "u"; match what
		 * follows the "u".  If that fails, try it as an SUS
		 * integer type.
		 */
		m->type = get_type(type_tbl, l + 1, &l);
		if (m->type == FILE_INVALID) {
			/*
			 * Not a keyword type; parse it as an SUS type,
			 * 'u' possibly followed by a number or C/S/L.
			 */
			m->type = get_standard_integer_type(l, &l);
		}
		/* It's unsigned. */
		if (m->type != FILE_INVALID)
			m->flag |= UNSIGNED;
	} else {
		/*
		 * Try it as a keyword type.  If that fails, try it as
		 * an SUS integer type if it begins with "d" or as an
		 * SUS string type if it begins with "s".  In any case,
		 * it's not unsigned.
		 */
		m->type = get_type(type_tbl, l, &l);
		if (m->type == FILE_INVALID) {
			/*
			 * Not a keyword type; parse it as an SUS type,
			 * either 'd' possibly followed by a number or
			 * C/S/L, or just 's'.
			 */
			if (*l == 'd')
				m->type = get_standard_integer_type(l, &l);
			else if (*l == 's'
			    && !isalpha(CAST(unsigned char, l[1]))) {
				m->type = FILE_STRING;
				++l;
			}
		}
	}

	if (m->type == FILE_INVALID) {
		/* Not found - try it as a special keyword. */
		m->type = get_type(special_tbl, l, &l);
	}

	if (m->type == FILE_INVALID) {
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("type `%s' invalid\n", l);
		return -1;
	}

	if (m->type == FILE_NAME && cont_level != 0) {
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("`name%s' entries can only be declared at top level\n", l);
		return -1;
	}

	/* New-style anding: "0 byte&0x80 =0x80 dynamically linked" */
	/* New and improved: ~ & | ^ + - * / % -- exciting, isn't it? */

	m->mask_op = 0;
	if (*l == '~') {
		if (!IS_STRING(m->type))
			m->mask_op |= FILE_OPINVERSE;
		else if (ms->flags & MAGIC_CHECK)
			log_warn_ex("'~' invalid for string types\n");
		++l;
	}
	m->_u._s._count = 0;
	m->_u._s._flags = m->type == FILE_PSTRING ? PSTRING_1_LE : 0;
	if ((op = get_op(*l)) != -1) {
		if (IS_STRING(m->type)) {
			int r;

			if (op != FILE_OPDIVIDE) {
				if (ms->flags & MAGIC_CHECK)
					log_warn_ex("invalid string/indirect op: `%c'\n", *t);
				return -1;
			}

			if (m->type == FILE_INDIRECT)
				r = parse_indirect_modifier(ms, m, &l);
			else
				r = parse_string_modifier(ms, m, &l);
			if (r == -1)
				return -1;
		} else
			parse_op_modifier(ms, m, &l, op);
	}

	/*
	 * We used to set mask to all 1's here, instead let's just not do
	 * anything if mask = 0 (unless you have a better idea)
	 */
	EATAB;

	switch (*l) {
	case '>':
	case '<':
  		m->reln = *l;
  		++l;
		if (*l == '=') {
			if (ms->flags & MAGIC_CHECK) {
				log_warn_ex("%c= not supported", m->reln);
				return -1;
			}
		   ++l;
		}
		break;
	/* Old-style anding: "0 byte &0x80 dynamically linked" */
	case '&':
	case '^':
	case '=':
  		m->reln = *l;
  		++l;
		if (*l == '=') {
		   /* HP compat: ignore &= etc. */
		   ++l;
		}
		break;
	case '!':
		m->reln = *l;
		++l;
		break;
	default:
  		m->reln = '=';	/* the default relation */
		if (*l == 'x' && ((isascii(CAST(unsigned char, l[1])) &&
		    isspace(CAST(unsigned char, l[1]))) || !l[1])) {
			m->reln = *l;
			++l;
		}
		break;
	}
	/*
	 * Grab the value part, except for an 'x' reln.
	 */
	if (m->reln != 'x' && getvalue(ms, m, &l, action))
		return -1;

	/*
	 * TODO finish this macro and start using it!
	 * #define offsetcheck {if (offset > ms->bytes_max -1)
	 *	magwarn("offset too big"); }
	 */

	/*
	 * Now get last part - the description
	 */
	EATAB;
	if (l[0] == '\b') {
		++l;
		m->flag |= NOSPACE;
	} else if ((l[0] == '\\') && (l[1] == 'b')) {
		++l;
		++l;
		m->flag |= NOSPACE;
	}
	for (i = 0; (m->desc[i++] = *l++) != '\0' && i < sizeof(m->desc); )
		continue;
	if (i == sizeof(m->desc)) {
		m->desc[sizeof(m->desc) - 1] = '\0';
		if (ms->flags & MAGIC_CHECK)
			log_warn_ex("description `%s' truncated", m->desc);
	}

        /*
	 * We only do this check while compiling, or if any of the magic
	 * files were not compiled.
         */
        if (ms->flags & MAGIC_CHECK) {
		if (check_format(ms, m) == -1)
			return -1;
	}

	m->mimetype[0] = '\0';		/* initialise MIME type to none */
	return 0;
}

int addentry(struct magic_set *ms, struct magic_entry *me,struct magic_entry_set *mset)
{
	size_t i = me->mp->type == FILE_NAME ? 1 : 0;
	if (mset[i].count == mset[i].max) {
		struct magic_entry *mp;

		mset[i].max += ALLOC_INCR;
		if ((mp = CAST(struct magic_entry *,
		    realloc(mset[i].me, sizeof(*mp) * mset[i].max))) ==
		    NULL) {
			log_warn_ex("relloc failed\n");
			return -1;
		}
		memset(&mp[mset[i].count], 0, sizeof(*mp) *ALLOC_INCR);
		mset[i].me = mp;
	}
	mset[i].me[mset[i].count++] = *me;
	memset(me, 0, sizeof(*me));
	return 0;
}

void load_1(struct magic_set *ms, int action, const char *fn, int *errs,struct magic_entry_set *mset)
{
	size_t lineno = 0, llen = 0;
	char *line = NULL;
	ssize_t len;
	struct magic_entry me;
	FILE *f = fopen(ms->file = fn, "r");
	if (f == NULL) {
		if (errno != ENOENT) 
			log_warn_ex("cannot read magic file %s, errno=%d",fn, errno);
		log_warn_ex("fopen open failed, path=%s\n",ms->file);
		(*errs)++;
		return;
	}

	memset(&me, 0, sizeof(me));
	/* read and parse this file */
	for (ms->line = 1; (len = getline(&line, &llen, f)) != -1;
	    ms->line++) {
		if (len == 0) /* null line, garbage, etc */
			continue;
		if (line[len - 1] == '\n') {
			lineno++;
			line[len - 1] = '\0'; /* delete newline */
		}
		switch (line[0]) {
			case '\0':	/* empty, do not parse */
			case '#':	/* comment, do not parse */
				continue;
			case '!':
				if (line[1] == ':') {
					size_t i;

					for (i = 0; bang[i].name != NULL; i++) {
						if (CAST(size_t, len - 2) > bang[i].len &&
						    memcmp(bang[i].name, line + 2,
						    bang[i].len) == 0)
							break;
					}
					if (bang[i].name == NULL) {
						log_warn_ex("Unknown !: entry %s", line);
						(*errs)++;
						continue;
					}
					if (me.mp == NULL) {
						log_warn_ex("No current entry for :!%s type",bang[i].name);
						(*errs)++;
						continue;
					}
					if ((*bang[i].fun)(ms, &me,
					    line + bang[i].len + 2, 
					    len - bang[i].len - 2) != 0) {
						log_warn_ex("band fun failed, name:%s\n",bang[i].name);
						(*errs)++;
						continue;
					}
					continue;
				}

			default:
			again_parse:
				switch (parse(ms, &me, line, lineno, action)) {
					case 0:
						continue;
					case 1:
						addentry(ms, &me, mset);
						goto again_parse;
					default:
						log_warn_ex("parse failed\n");
						(*errs)++;
						break;
				}
		}
	}
	if (me.mp)
		addentry(ms, &me, mset);
	free(line);
	fclose(f);

	return;
}

int file_looks_utf8(const unsigned char *buf, size_t nbytes, file_unichar_t *ubuf,size_t *ulen)
{
	size_t i;
	int n;
	file_unichar_t c;
	int gotone = 0, ctrl = 0;

	if (ubuf)
		*ulen = 0;

	for (i = 0; i < nbytes; i++) {
		if ((buf[i] & 0x80) == 0) {	   /* 0xxxxxxx is plain ASCII */
			/*
			 * Even if the whole file is valid UTF-8 sequences,
			 * still reject it if it uses weird control characters.
			 */

			if (text_chars[buf[i]] != T)
				ctrl = 1;

			if (ubuf)
				ubuf[(*ulen)++] = buf[i];
		} else if ((buf[i] & 0x40) == 0) { /* 10xxxxxx never 1st byte */
			return -1;
		} else {			   /* 11xxxxxx begins UTF-8 */
			int following;
			uint8_t x = first[buf[i]];
			const struct accept_range *ar =
			    &accept_ranges[(unsigned int)x >> 4];
			if (x == XX)
				return -1;

			if ((buf[i] & 0x20) == 0) {		/* 110xxxxx */
				c = buf[i] & 0x1f;
				following = 1;
			} else if ((buf[i] & 0x10) == 0) {	/* 1110xxxx */
				c = buf[i] & 0x0f;
				following = 2;
			} else if ((buf[i] & 0x08) == 0) {	/* 11110xxx */
				c = buf[i] & 0x07;
				following = 3;
			} else if ((buf[i] & 0x04) == 0) {	/* 111110xx */
				c = buf[i] & 0x03;
				following = 4;
			} else if ((buf[i] & 0x02) == 0) {	/* 1111110x */
				c = buf[i] & 0x01;
				following = 5;
			} else
				return -1;

			for (n = 0; n < following; n++) {
				i++;
				if (i >= nbytes)
					goto done;

				if (n == 0 &&
				     (buf[i] < ar->lo || buf[i] > ar->hi))
					return -1;

				if ((buf[i] & 0x80) == 0 || (buf[i] & 0x40))
					return -1;

				c = (c << 6) + (buf[i] & 0x3f);
			}

			if (ubuf)
				ubuf[(*ulen)++] = c;
			gotone = 1;
		}
	}
done:
	return ctrl ? 0 : (gotone ? 2 : 1);
}

void set_test_type(struct magic *mstart, struct magic *m)
{
	switch (m->type) {
	case FILE_BYTE:
	case FILE_SHORT:
	case FILE_LONG:
	case FILE_DATE:
	case FILE_BESHORT:
	case FILE_BELONG:
	case FILE_BEDATE:
	case FILE_LESHORT:
	case FILE_LELONG:
	case FILE_LEDATE:
	case FILE_LDATE:
	case FILE_BELDATE:
	case FILE_LELDATE:
	case FILE_MEDATE:
	case FILE_MELDATE:
	case FILE_MELONG:
	case FILE_QUAD:
	case FILE_LEQUAD:
	case FILE_BEQUAD:
	case FILE_QDATE:
	case FILE_LEQDATE:
	case FILE_BEQDATE:
	case FILE_QLDATE:
	case FILE_LEQLDATE:
	case FILE_BEQLDATE:
	case FILE_QWDATE:
	case FILE_LEQWDATE:
	case FILE_BEQWDATE:
	case FILE_FLOAT:
	case FILE_BEFLOAT:
	case FILE_LEFLOAT:
	case FILE_DOUBLE:
	case FILE_BEDOUBLE:
	case FILE_LEDOUBLE:
	case FILE_DER:
	case FILE_GUID:
	case FILE_OFFSET:
		mstart->flag |= BINTEST;
		break;
	case FILE_STRING:
	case FILE_PSTRING:
	case FILE_BESTRING16:
	case FILE_LESTRING16:
		/* Allow text overrides */
		if (mstart->_u._s._flags & STRING_TEXTTEST)
			mstart->flag |= TEXTTEST;
		else
			mstart->flag |= BINTEST;
		break;
	case FILE_REGEX:
	case FILE_SEARCH:
		/* Check for override */
		if (mstart->_u._s._flags & STRING_BINTEST)
			mstart->flag |= BINTEST;
		if (mstart->_u._s._flags & STRING_TEXTTEST)
			mstart->flag |= TEXTTEST;

		if (mstart->flag & (TEXTTEST|BINTEST))
			break;

		/* binary test if pattern is not text */
		if (file_looks_utf8(m->value.us, CAST(size_t, m->vallen), NULL,
		    NULL) <= 0)
			mstart->flag |= BINTEST;
		else
			mstart->flag |= TEXTTEST;
		break;
	case FILE_DEFAULT:
		/* can't deduce anything; we shouldn't see this at the
		   top level anyway */
		break;
	case FILE_INVALID:
	default:
		/* invalid search type, but no need to complain here */
		break;
	}
}

uint32_t set_text_binary(struct magic_set *ms, struct magic_entry *me, uint32_t nme,uint32_t starttest)
{
	static const char text[] = "text";
	static const char binary[] = "binary";
	static const size_t len = sizeof(text);

	uint32_t i = starttest;

	do {
		set_test_type(me[starttest].mp, me[i].mp);
		if ((ms->flags & MAGIC_DEBUG) == 0)
			continue;
		log_warn_ex("%s%s%s: %s\n",
				me[i].mp->mimetype,
				me[i].mp->mimetype[0] == '\0' ? "" : "; ",
				me[i].mp->desc[0] ? me[i].mp->desc : "(no description)",
				me[i].mp->flag & BINTEST ? binary : text);
		if (me[i].mp->flag & BINTEST) {
			char *p = strstr(me[i].mp->desc, text);
			if (p && (p == me[i].mp->desc ||
			    isspace(CAST(unsigned char, p[-1]))) &&
			    (p + len - me[i].mp->desc == MAXstring
			    || (p[len] == '\0' ||
			    isspace(CAST(unsigned char, p[len])))))
				log_warn_ex("*** Possible binary test for text type\n");
		}
	} while (++i < nme && me[i].mp->cont_level != 0);
	return i;
}

size_t nonmagic(const char *str)
{
	const char *p;
	size_t rv = 0;

	for (p = str; *p; p++)
		switch (*p) {
		case '\\':	/* Escaped anything counts 1 */
			if (!*++p)
				p--;
			rv++;
			continue;
		case '?':	/* Magic characters count 0 */
		case '*':
		case '.':
		case '+':
		case '^':
		case '$':
			continue;
		case '[':	/* Bracketed expressions count 1 the ']' */
			while (*p && *p != ']')
				p++;
			p--;
			continue;
		case '{':	/* Braced expressions count 0 */
			while (*p && *p != '}')
				p++;
			if (!*p)
				p--;
			continue;
		default:	/* Anything else counts 1 */
			rv++;
			continue;
		}

	return rv == 0 ? 1 : rv;	/* Return at least 1 */
}

size_t apprentice_magic_strength(const struct magic *m)
{
#define MULT 10U
	size_t ts, v;
	ssize_t val = 2 * MULT;	/* baseline strength */

	switch (m->type) {
	case FILE_DEFAULT:	/* make sure this sorts last */
		if (m->factor_op != FILE_FACTOR_OP_NONE)
			log_error_ex("m->factor_op values is not support\n");
		return 0;

	case FILE_BYTE:
	case FILE_SHORT:
	case FILE_LESHORT:
	case FILE_BESHORT:
	case FILE_LONG:
	case FILE_LELONG:
	case FILE_BELONG:
	case FILE_MELONG:
	case FILE_DATE:
	case FILE_LEDATE:
	case FILE_BEDATE:
	case FILE_MEDATE:
	case FILE_LDATE:
	case FILE_LELDATE:
	case FILE_BELDATE:
	case FILE_MELDATE:
	case FILE_FLOAT:
	case FILE_BEFLOAT:
	case FILE_LEFLOAT:
	case FILE_QUAD:
	case FILE_BEQUAD:
	case FILE_LEQUAD:
	case FILE_QDATE:
	case FILE_LEQDATE:
	case FILE_BEQDATE:
	case FILE_QLDATE:
	case FILE_LEQLDATE:
	case FILE_BEQLDATE:
	case FILE_QWDATE:
	case FILE_LEQWDATE:
	case FILE_BEQWDATE:
	case FILE_DOUBLE:
	case FILE_BEDOUBLE:
	case FILE_LEDOUBLE:
	case FILE_GUID:
	case FILE_OFFSET:
		ts = typesize(m->type);
		if (ts == FILE_BADSIZE)
			log_error_ex("Bad ts %u\n", (unsigned int)ts);
		val += ts * MULT;
		break;

	case FILE_PSTRING:
	case FILE_STRING:
		val += m->vallen * MULT;
		break;

	case FILE_BESTRING16:
	case FILE_LESTRING16:
		val += m->vallen * MULT / 2;
		break;

	case FILE_SEARCH:
		if (m->vallen == 0)
			break;
		val += m->vallen * MAX(MULT / m->vallen, 1);
		break;

	case FILE_REGEX:
		v = nonmagic(m->value.s);
		val += v * MAX(MULT / v, 1);
		break;

	case FILE_INDIRECT:
	case FILE_NAME:
	case FILE_USE:
		break;

	case FILE_DER:
		val += MULT;
		break;

	default:
		log_error_ex("Bad type %d\n", m->type);
		
	}

	switch (m->reln) {
	case 'x':	/* matches anything penalize */
	case '!':       /* matches almost anything penalize */
		val = 0;
		break;

	case '=':	/* Exact match, prefer */
		val += MULT;
		break;

	case '>':
	case '<':	/* comparison match reduce strength */
		val -= 2 * MULT;
		break;

	case '^':
	case '&':	/* masking bits, we could count them too */
		val -= MULT;
		break;

	default:
		log_error_ex( "Bad relation %c\n", m->reln);
		
	}

	switch (m->factor_op) {
	case FILE_FACTOR_OP_NONE:
		break;
	case FILE_FACTOR_OP_PLUS:
		val += m->factor;
		break;
	case FILE_FACTOR_OP_MINUS:
		val -= m->factor;
		break;
	case FILE_FACTOR_OP_TIMES:
		val *= m->factor;
		break;
	case FILE_FACTOR_OP_DIV:
		val /= m->factor;
		break;
	default:
		log_error_ex( "bad factor_op %c\n", m->factor_op);
	}

	if (val <= 0)	/* ensure we only return 0 for FILE_DEFAULT */
		val = 1;

	/*
	 * Magic entries with no description get a bonus because they depend
	 * on subsequent magic entries to print something.
	 */
	if (m->desc[0] == '\0')
		val++;
	return val;
}

int apprentice_sort(const void *a, const void *b)
{
	const struct magic_entry *ma = CAST(const struct magic_entry *, a);
	const struct magic_entry *mb = CAST(const struct magic_entry *, b);
	size_t sa = apprentice_magic_strength(ma->mp);
	size_t sb = apprentice_magic_strength(mb->mp);
	if (sa == sb)
		return 0;
	else if (sa > sb)
		return -1;
	else
		return 1;
}

void set_last_default(struct magic_set *ms, struct magic_entry *me, uint32_t nme)
{
	uint32_t i;
	for (i = 0; i < nme; i++) {
		if (me[i].mp->cont_level == 0 &&
		    me[i].mp->type == FILE_DEFAULT) {
			while (++i < nme)
				if (me[i].mp->cont_level == 0)
					break;
			if (i != nme) {
				/* XXX - Ugh! */
				ms->line = me[i].mp->lineno;
				log_warn_ex("level 0 \"default\" did not sort last");
			}
			return;
		}
	}
}

int coalesce_entries(struct magic_set *ms, struct magic_entry *me, uint32_t nme,struct magic **ma, uint32_t *nma)
{
	uint32_t i, mentrycount = 0;
	size_t slen;

	for (i = 0; i < nme; i++)
		mentrycount += me[i].cont_count;

	slen = sizeof(**ma) * mentrycount;
	if ((*ma = CAST(struct magic *, malloc(slen))) == NULL) {
		log_warn_ex("malloc failed\n");
		return -1;
	}

	mentrycount = 0;
	for (i = 0; i < nme; i++) {
		memcpy(*ma + mentrycount, me[i].mp,me[i].cont_count * sizeof(**ma));
		mentrycount += me[i].cont_count;
	}
	*nma = mentrycount;
	return 0;
}

void magic_entry_free(struct magic_entry *me, uint32_t nme)
{
	uint32_t i;
	if (me == NULL)
		return;
	for (i = 0; i < nme; i++)
		free(me[i].mp);
	free(me);

	return;
}

struct magic_map *apprentice_load(struct magic_set *ms, const char *fn, int action)
{
	int errs = 0;
	uint32_t i, j;
	size_t files = 0, maxfiles = 0;
	char **filearr = NULL, *mfn;
	struct stat st;
	struct magic_map *map;
	struct magic_entry_set mset[MAGIC_SETS];
	DIR *dir;
	struct dirent *d;

	memset(mset, 0, sizeof(mset));
	ms->flags |= MAGIC_CHECK;	/* Enable checks for parsed files */

	if (!(map = CAST(struct magic_map *, calloc(1, sizeof(*map))))) {
		log_warn_ex("calloc failed\n");
		return NULL;
	}

	map->type = MAP_TYPE_MALLOC;


	/* load directory or file */
	if (stat(fn, &st) == 0 && S_ISDIR(st.st_mode)) {
		dir = opendir(fn);
		if (!dir) {
			log_warn_ex("opendir failed, errno=%d\n", errno);
			errs++;
			goto out;
		}
		while ((d = readdir(dir)) != NULL) {
			if (d->d_name[0] == '.')
				continue;
			if (asprintf(&mfn, "%s/%s", fn, d->d_name) < 0) {
				log_warn_ex("asprintf failed, d->d_name = %s\n", d->d_name);
				errs++;
				closedir(dir);
				goto out;
			}
			if (stat(mfn, &st) == -1 || !S_ISREG(st.st_mode)) {
				free(mfn);
				continue;
			}
			if (files >= maxfiles) {
				size_t mlen;
				char **nfilearr;
				maxfiles = (maxfiles + 1) * 2;
				mlen = maxfiles * sizeof(*filearr);
				if ((nfilearr = CAST(char **,realloc(filearr, mlen))) == NULL) {
					log_warn_ex("realloc failed\n");
					free(mfn);
					closedir(dir);
					errs++;
					goto out;
				}
				filearr = nfilearr;
			}
			filearr[files++] = mfn;
		}
		closedir(dir);
		if (filearr) {
			qsort(filearr, files, sizeof(*filearr), cmpstrp);
			for (i = 0; i < files; i++) {
				load_1(ms, action, filearr[i], &errs, mset);
				free(filearr[i]);
			}
			free(filearr);
			filearr = NULL;
		}
	} else
		load_1(ms, action, fn, &errs, mset);
		
	if (errs) {
		log_warn_ex("error has happend\n",errs);
		goto out;
	}

	for (j = 0; j < MAGIC_SETS; j++) {
		/* Set types of tests */
		for (i = 0; i < mset[j].count; ) {
			if (mset[j].me[i].mp->cont_level != 0) {
				i++;
				continue;
			}
			i = set_text_binary(ms, mset[j].me, mset[j].count, i);
		}
		if (mset[j].me)
			qsort(mset[j].me, mset[j].count, sizeof(*mset[j].me),apprentice_sort);

		/*
		 * Make sure that any level 0 "default" line is last
		 * (if one exists).
		 */
		set_last_default(ms, mset[j].me, mset[j].count);

		/* coalesce per file arrays into a single one, if needed */
		if (mset[j].count == 0)
			continue;
		      
		if (coalesce_entries(ms, mset[j].me, mset[j].count,
		    &map->magic[j], &map->nmagic[j]) == -1) {
			errs++;
			goto out;
		}
	}

out:
	free(filearr);
	for (j = 0; j < MAGIC_SETS; j++)
		magic_entry_free(mset[j].me, mset[j].count);

	if (errs) {
		apprentice_unmap(map);
		return NULL;
	}
	return map;
}

int apprentice_compile(struct magic_set *ms, struct magic_map *map, const char *fn, char *out_magic)
{
	static const size_t nm = sizeof(*map->nmagic) * MAGIC_SETS;
	static const size_t m = sizeof(**map->magic);
	int fd = -1;
	size_t len;
	char *dbname;
	int rv = -1;
	uint32_t i;
	union {
		struct magic m;
		uint32_t h[2 + MAGIC_SETS];
	} hdr;

	log_debug_ex("entry fn:%s\n", fn);

	dbname = out_magic;

	if (dbname == NULL)
		goto out;

	log_debug_ex("write magic:%s\n", dbname);
	if ((fd = open(dbname, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1)
	{
		log_warn_ex("cannot open `%s', errno=%d\n", dbname, errno);
		goto out;
	}
	memset(&hdr, 0, sizeof(hdr));
	hdr.h[0] = MAGICNO;
	hdr.h[1] = VERSIONNO;
	memcpy(hdr.h + 2, map->nmagic, nm);

	if (write(fd, &hdr, sizeof(hdr)) != CAST(ssize_t, sizeof(hdr))) {
		log_warn_ex("error writing `%s', errno=%d\n", dbname, errno);
		goto out2;
	}

	for (i = 0; i < MAGIC_SETS; i++) {
		len = m * map->nmagic[i];
		if (write(fd, map->magic[i], len) != CAST(ssize_t, len)) {
			log_warn_ex("error writing `%s', errno=%d\n", dbname, errno);
			goto out2;
		}
	}

	rv = 0;
out2:
	if (fd != -1)
		(void)close(fd);
out:
	apprentice_unmap(map);
	return rv;
}

int apprentice_1(struct magic_set *ms, const char *fn, int action, char *out_magic)
{
	struct magic_map *map;

	if (magicsize != FILE_MAGICSIZE) {
		log_warn_ex("magic size is not equal, size= %u\n", (unsigned int)magicsize);
		return -1;
	}

	if (action == FILE_COMPILE) {
		map = apprentice_load(ms, fn, action);
		if (map == NULL) {
			log_warn_ex("apprentice_load failed\n");
			return -1;
		}
		return apprentice_compile(ms, map, fn, out_magic);
	}

	return 0;
}

int file_apprentice(struct magic_set *ms, const char *fn, int action, char *out_magic)
{
	char *p, *mfn;
	int fileerr, errs = -1;
	size_t i, j;

	file_reset(ms, 0);

	init_file_tables();

	if (!(mfn = strdup(fn))) {
		log_warn_ex("strdup failed\n");
		return -1;
	}

	for (i = 0; i < MAGIC_SETS; i++) {
		mlist_free(ms->mlist[i]);
		if ((ms->mlist[i] = mlist_alloc()) == NULL) {
			log_warn_ex("mlist_alloc failed\n");
			for (j = 0; j < i; j++) {
				mlist_free(ms->mlist[j]);
				ms->mlist[j] = NULL;
			}
			free(mfn);
			return -1;
		}
	}
	
	fn = mfn;

	while (fn) {
		p = strchr(fn, PATHSEP);
		if (p)
			*p++ = '\0';
		if (*fn == '\0')
			break;
		fileerr = apprentice_1(ms, fn, action, out_magic);
		errs = MAX(errs, fileerr);
		fn = p;
	}

	free(mfn);

	if (errs == -1) {
		for (i = 0; i < MAGIC_SETS; i++) {
			mlist_free(ms->mlist[i]);
			ms->mlist[i] = NULL;
		}

		log_warn_ex("could not find any valid magic files\n");
		return -1;
	}

	switch (action) {
		case FILE_LOAD:
		case FILE_COMPILE:
		case FILE_CHECK:
		case FILE_LIST:
			return 0;
		default:
			return -1;
	}

	return 0;
}

int magic_compile(struct magic_set *ms, const char *magicfile, char *out_magic)
{
	if (!ms)
		return -1;
	return file_apprentice(ms, magicfile, FILE_COMPILE, out_magic);
}

void file_ms_free(struct magic_set *ms)
{
	size_t i;
	if (ms == NULL)
		return;
	for (i = 0; i < MAGIC_SETS; i++)
		mlist_free(ms->mlist[i]);
	free(ms->o.pbuf);
	free(ms->o.buf);
	free(ms->c.li);
	free(ms);
}

void magic_close(struct magic_set *ms)
{
	if (ms == NULL)
		return;
	file_ms_free(ms);
	return;
}


