#ifndef __file_h__
#define __file_h__

#include <file_config.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <locale.h>
#include "opbox/utils.h"

#define USE_C_LOCALE
#define ENABLE_CONDITIONALS
#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#define MAGIC "/etc/magic"
#define SIZE_T_FORMAT "z"
#define INT64_T_FORMAT "ll"
#define INTMAX_T_FORMAT "j"
#define PATHSEP	':'
#define private static
#define protected
#define public
#define protected
#define __arraycount(a) (sizeof(a) / sizeof(a[0]))
#define	__GNUC_PREREQ__(x, y)	0
#define FD_CLOEXEC 1
#define FILE_BADSIZE CAST(size_t, ~0ul)
#define MAXDESC	64		
#define MAXMIME	80		
#define MAXstring 128	
#define MAGICNO		0xF11E041C
#define VERSIONNO	16
#define FILE_MAGICSIZE	376
#define FILE_GUID_SIZE	sizeof("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")
#define	FILE_LOAD	0
#define FILE_CHECK	1
#define FILE_COMPILE	2
#define FILE_LIST	3
#define INDIR		0x01	/* if '(...)' appears */
#define OFFADD		0x02	/* if '>&' or '>...(&' appears */
#define INDIROFFADD	0x04	/* if '>&(' appears */
#define UNSIGNED	0x08	/* comparison is unsigned */
#define NOSPACE		0x10	/* suppress space character before output */
#define BINTEST		0x20	/* test is for a binary type (set only for top-level tests) */
#define TEXTTEST	0x40	/* for passing to file_softmagic */
#define OFFNEGATIVE	0x80	/* relative to the end of file */
#define 			FILE_INVALID	0
#define 			FILE_BYTE	1
#define				FILE_SHORT	2
#define				FILE_DEFAULT	3
#define				FILE_LONG	4
#define				FILE_STRING	5
#define				FILE_DATE	6
#define				FILE_BESHORT	7
#define				FILE_BELONG	8
#define				FILE_BEDATE	9
#define				FILE_LESHORT	10
#define				FILE_LELONG	11
#define				FILE_LEDATE	12
#define				FILE_PSTRING	13
#define				FILE_LDATE	14
#define				FILE_BELDATE	15
#define				FILE_LELDATE	16
#define				FILE_REGEX	17
#define				FILE_BESTRING16	18
#define				FILE_LESTRING16	19
#define				FILE_SEARCH	20
#define				FILE_MEDATE	21
#define				FILE_MELDATE	22
#define				FILE_MELONG	23
#define				FILE_QUAD	24
#define				FILE_LEQUAD	25
#define				FILE_BEQUAD	26
#define				FILE_QDATE	27
#define				FILE_LEQDATE	28
#define				FILE_BEQDATE	29
#define				FILE_QLDATE	30
#define				FILE_LEQLDATE	31
#define				FILE_BEQLDATE	32
#define				FILE_FLOAT	33
#define				FILE_BEFLOAT	34
#define				FILE_LEFLOAT	35
#define				FILE_DOUBLE	36
#define				FILE_BEDOUBLE	37
#define				FILE_LEDOUBLE	38
#define				FILE_BEID3	39
#define				FILE_LEID3	40
#define				FILE_INDIRECT	41
#define				FILE_QWDATE	42
#define				FILE_LEQWDATE	43
#define				FILE_BEQWDATE	44
#define				FILE_NAME	45
#define				FILE_USE	46
#define				FILE_CLEAR	47
#define				FILE_DER	48
#define				FILE_GUID	49
#define				FILE_OFFSET	50
#define				FILE_NAMES_SIZE	51 /* size of array to contain all names */

#define IS_STRING(t) \
	((t) == FILE_STRING || \
	 (t) == FILE_PSTRING || \
	 (t) == FILE_BESTRING16 || \
	 (t) == FILE_LESTRING16 || \
	 (t) == FILE_REGEX || \
	 (t) == FILE_SEARCH || \
	 (t) == FILE_INDIRECT || \
	 (t) == FILE_NAME || \
	 (t) == FILE_USE)

#define FILE_FMT_NONE 0
#define FILE_FMT_NUM  1 /* "cduxXi" */
#define FILE_FMT_STR  2 /* "s" */
#define FILE_FMT_QUAD 3 /* "ll" */
#define FILE_FMT_FLOAT 4 /* "eEfFgG" */
#define FILE_FMT_DOUBLE 5 /* "eEfFgG" */
#define		FILE_FACTOR_OP_PLUS	'+'
#define		FILE_FACTOR_OP_MINUS	'-'
#define		FILE_FACTOR_OP_TIMES	'*'
#define		FILE_FACTOR_OP_DIV	'/'
#define		FILE_FACTOR_OP_NONE	'\0'
#define				FILE_OPS	"&|^+-*/%"
#define				FILE_OPAND	0
#define				FILE_OPOR	1
#define				FILE_OPXOR	2
#define				FILE_OPADD	3
#define				FILE_OPMINUS	4
#define				FILE_OPMULTIPLY	5
#define				FILE_OPDIVIDE	6
#define				FILE_OPMODULO	7
#define				FILE_OPS_MASK	0x07 /* mask for above ops */
#define				FILE_UNUSED_1	0x08
#define				FILE_UNUSED_2	0x10
#define				FILE_OPSIGNED	0x20
#define				FILE_OPINVERSE	0x40
#define				FILE_OPINDIRECT	0x80
#define				COND_NONE	0
#define				COND_IF		1
#define				COND_ELIF	2
#define				COND_ELSE	3
#define num_mask _u._mask
#define str_range _u._s._count
#define str_flags _u._s._flags
#define BIT(A)   (1 << (A))
#define STRING_COMPACT_WHITESPACE		BIT(0)
#define STRING_COMPACT_OPTIONAL_WHITESPACE	BIT(1)
#define STRING_IGNORE_LOWERCASE			BIT(2)
#define STRING_IGNORE_UPPERCASE			BIT(3)
#define REGEX_OFFSET_START			BIT(4)
#define STRING_TEXTTEST				BIT(5)
#define STRING_BINTEST				BIT(6)
#define PSTRING_1_BE				BIT(7)
#define PSTRING_1_LE				BIT(7)
#define PSTRING_2_BE				BIT(8)
#define PSTRING_2_LE				BIT(9)
#define PSTRING_4_BE				BIT(10)
#define PSTRING_4_LE				BIT(11)
#define REGEX_LINE_COUNT			BIT(11)
#define PSTRING_LEN	(PSTRING_1_BE|PSTRING_2_LE|PSTRING_2_BE|PSTRING_4_LE|PSTRING_4_BE)
#define PSTRING_LENGTH_INCLUDES_ITSELF		BIT(12)
#define	STRING_TRIM				BIT(13)
#define CHAR_COMPACT_WHITESPACE			'W'
#define CHAR_COMPACT_OPTIONAL_WHITESPACE	'w'
#define CHAR_IGNORE_LOWERCASE			'c'
#define CHAR_IGNORE_UPPERCASE			'C'
#define CHAR_REGEX_OFFSET_START			's'
#define CHAR_TEXTTEST				't'
#define	CHAR_TRIM				'T'
#define CHAR_BINTEST				'b'
#define CHAR_PSTRING_1_BE			'B'
#define CHAR_PSTRING_1_LE			'B'
#define CHAR_PSTRING_2_BE			'H'
#define CHAR_PSTRING_2_LE			'h'
#define CHAR_PSTRING_4_BE			'L'
#define CHAR_PSTRING_4_LE			'l'
#define CHAR_PSTRING_LENGTH_INCLUDES_ITSELF     'J'
#define STRING_IGNORE_CASE		(STRING_IGNORE_LOWERCASE|STRING_IGNORE_UPPERCASE)
#define STRING_DEFAULT_RANGE		100
#define	INDIRECT_RELATIVE			BIT(0)
#define	CHAR_INDIRECT_RELATIVE			'r'
#define CAST(T, b)	((T)(b))
#define RCAST(T, b)	((T)(uintptr_t)(b))
#define CCAST(T, b)	((T)(uintptr_t)(b))
#define MAGIC_SETS	2
#define 		EVENT_HAD_ERR		0x01
#define FILE_BYTES_MAX (1024 * 1024)	/* how much of the file to look at */
#define	FILE_ELF_NOTES_MAX		256
#define	FILE_ELF_PHNUM_MAX		2048
#define	FILE_ELF_SHNUM_MAX		32768
#define	FILE_INDIR_MAX			50
#define	FILE_NAME_MAX			50
#define	FILE_REGEX_MAX			8192
#define	FILE_ENCODING_MAX		(64 * 1024)
#define FILE_T_LOCAL	1
#define FILE_T_WINDOWS	2
#define QUICK
#define O_BINARY	0

#define FILE_RCSID(id) \
static const char rcsid[] __attribute__((__used__)) = id;

#define __RCSID(a)

typedef unsigned long file_unichar_t;
extern const char *file_names[];
extern const size_t file_nnames;
typedef struct {
	const char *pat;
	locale_t old_lc_ctype;
	locale_t c_lc_ctype;

	int rc;
	regex_t rx;
} file_regex_t;

typedef struct {
	char *buf;
	size_t blen;
	uint32_t offset;
} file_pushbuf_t;

struct buffer {
	int fd;
	struct stat st;
	const void *fbuf;
	size_t flen;
	off_t eoff;
	void *ebuf;
	size_t elen;
};

union VALUETYPE {
	uint8_t b;
	uint16_t h;
	uint32_t l;
	uint64_t q;
	uint8_t hs[2];	/* 2 bytes of a fixed-endian "short" */
	uint8_t hl[4];	/* 4 bytes of a fixed-endian "long" */
	uint8_t hq[8];	/* 8 bytes of a fixed-endian "quad" */
	char s[MAXstring];	/* the search string or regex pattern */
	unsigned char us[MAXstring];
	uint64_t guid[2];
	float f;
	double d;
};

struct magic {
	/* Word 1 */
	uint16_t cont_level;	/* level of ">" */
	uint8_t flag;
	uint8_t factor;
	/* Word 2 */
	uint8_t reln;		/* relation (0=eq, '>'=gt, etc) */
	uint8_t vallen;		/* length of string value, if any */
	uint8_t type;		/* comparison type (FILE_*) */
	uint8_t in_type;	/* type of indirection */

	/* Word 3 */
	uint8_t in_op;		/* operator for indirection */
	uint8_t mask_op;	/* operator for mask */
	uint8_t cond;		/* conditional type */

	uint8_t factor_op;

	/* Word 4 */
	int32_t offset;		/* offset to magic number */
	/* Word 5 */
	int32_t in_offset;	/* offset from indirection */
	/* Word 6 */
	uint32_t lineno;	/* line number in magic file */
	/* Word 7,8 */
	union {
		uint64_t _mask;	/* for use with numeric and date types */
		struct {
			uint32_t _count;	/* repeat/line count */
			uint32_t _flags;	/* modifier flags */
		} _s;		/* for use with string types */
	} _u;

	/* Words 9-24 */
	union VALUETYPE value;	/* either number or string */
	/* Words 25-40 */
	char desc[MAXDESC];	/* description */
	/* Words 41-60 */
	char mimetype[MAXMIME]; /* MIME type */
	/* Words 61-62 */
	char apple[8];		/* APPLE CREATOR/TYPE */
	/* Words 63-78 */
	char ext[64];		/* Popular extensions */
};

/* list of magic entries */
struct mlist {
	struct magic *magic;		/* array of magic entries */
	uint32_t nmagic;		/* number of entries in array */
	void *map;			/* internal resources used by entry */
	struct mlist *next, *prev;
};

struct level_info {
	int32_t off;
	int got_match;
	int last_match;
	int last_cond;	/* used for error checking by parse() */
};

struct cont {
	size_t len;
	struct level_info *li;
};

struct magic_set {
	struct mlist *mlist[MAGIC_SETS];	/* list of regular entries */
	struct cont c;
	struct out {
		char *buf;		/* Accumulation buffer */
		size_t blen;		/* Length of buffer */
		char *pbuf;		/* Printable buffer */
	} o;
	uint32_t offset;			/* a copy of m->offset while we */
					/* are working on the magic entry */
	uint32_t eoffset;		/* offset from end of file */
	int error;
	int flags;			/* Control magic tests. */
	int event_flags;		/* Note things that happened. */

	const char *file;
	size_t line;			/* current magic line number */
	mode_t mode;			/* copy of current stat mode */

	/* data for searches */
	struct {
		const char *s;		/* start of search in original source */
		size_t s_len;		/* length of search region */
		size_t offset;		/* starting offset in source: XXX - should this be off_t? */
		size_t rm_len;		/* match length */
	} search;

	/* FIXME: Make the string dynamically allocated so that e.g.
	   strings matched in files can be longer than MAXstring */
	union VALUETYPE ms_value;	/* either number or string */
	uint16_t indir_max;
	uint16_t name_max;
	uint16_t elf_shnum_max;
	uint16_t elf_phnum_max;
	uint16_t elf_notes_max;
	uint16_t regex_max;
	size_t bytes_max;		/* number of bytes to read from file */
	size_t encoding_max;		/* bytes to look for encoding */
};

protected const char *file_fmttime(char *, size_t, uint64_t, int);
protected struct magic_set *file_ms_alloc(int);
protected void file_ms_free(struct magic_set *);
protected int file_default(struct magic_set *, size_t);
protected int file_buffer(struct magic_set *, int, struct stat *, const char *,const void *, size_t);
protected int file_fsmagic(struct magic_set *, const char *, struct stat *);
protected int file_pipe2file(struct magic_set *, int, const void *, size_t);
protected int file_vprintf(struct magic_set *, const char *, va_list ap);

protected int file_separator(struct magic_set *);
protected char *file_copystr(char *, size_t, size_t, const char *);
protected int file_checkfmt(char *, size_t, const char *);
protected size_t file_printedlen(const struct magic_set *);
protected int file_print_guid(char *, size_t, const uint64_t *);
protected int file_parse_guid(const char *, uint64_t *);
protected int file_replace(struct magic_set *, const char *, const char *);
protected int file_printf(struct magic_set *, const char *, ...);

protected int file_reset(struct magic_set *, int);
protected int file_tryelf(struct magic_set *, const struct buffer *);
protected int file_trycdf(struct magic_set *, const struct buffer *);

protected int file_zmagic(struct magic_set *, const struct buffer *,const char *);

protected int file_ascmagic(struct magic_set *, const struct buffer *,
    int);
protected int file_ascmagic_with_encoding(struct magic_set *,
    const struct buffer *, file_unichar_t *, size_t, const char *, const char *, int);
protected int file_encoding(struct magic_set *, const struct buffer *,
		file_unichar_t **, size_t *, const char **, const char **, const char **);
protected int file_is_json(struct magic_set *, const struct buffer *);
protected int file_is_csv(struct magic_set *, const struct buffer *, int);
protected int file_is_tar(struct magic_set *, const struct buffer *);
protected int file_softmagic(struct magic_set *, const struct buffer *,
		uint16_t *, uint16_t *, int, int);
protected int file_apprentice(struct magic_set *, const char *, int);
protected int buffer_apprentice(struct magic_set *, struct magic **,size_t *, size_t);
protected int file_magicfind(struct magic_set *, const char *, struct mlist *);
protected uint64_t file_signextend(struct magic_set *, struct magic *,uint64_t);
protected void file_badread(struct magic_set *);
protected void file_badseek(struct magic_set *);
protected void file_oomem(struct magic_set *, size_t);
protected void file_error(struct magic_set *, int, const char *, ...);
protected void file_magerror(struct magic_set *, const char *, ...);
protected void file_magwarn(struct magic_set *, const char *, ...);
protected void file_mdump(struct magic *);
protected void file_showstr(FILE *, const char *, size_t);
protected const char *file_getbuffer(struct magic_set *);
protected ssize_t sread(int, void *, size_t, int);
protected int file_check_mem(struct magic_set *, unsigned int);
protected int file_looks_utf8(const unsigned char *, size_t, file_unichar_t *,size_t *);
protected size_t file_pstring_length_size(struct magic_set *,const struct magic *);
protected size_t file_pstring_get_length(struct magic_set *,const struct magic *, const char *);
protected char * file_printable(char *, size_t, const char *, size_t);
protected int file_pipe_closexec(int *);
protected int file_clear_closexec(int);
protected char *file_strtrim(char *);
protected void buffer_init(struct buffer *, int, const struct stat *,const void *, size_t);
protected void buffer_fini(struct buffer *);
protected int buffer_fill(const struct buffer *);

protected int file_regcomp(file_regex_t *, const char *, int);
protected int file_regexec(file_regex_t *, const char *, size_t, regmatch_t *,
    int);
protected void file_regfree(file_regex_t *);
protected void file_regerror(file_regex_t *, int, struct magic_set *);
int file_main(int argc, char *argv[]);
int compile_main(int argc, char *argv[]);
protected file_pushbuf_t *file_push_buffer(struct magic_set *);
protected char  *file_pop_buffer(struct magic_set *, file_pushbuf_t *);
const char *fmtcheck(const char *, const char *);

#endif /* __file_h__ */
