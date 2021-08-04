#ifndef _FILE_COMPILE_H__
#define _FILE_COMPILE_H__
#include <stdint.h>
#include <sys/types.h>
#include <regex.h>
#include <locale.h>

#define	FILE_ELF_NOTES_MAX		256
#define	FILE_ELF_PHNUM_MAX		2048
#define	FILE_ELF_SHNUM_MAX		32768
#define	FILE_INDIR_MAX			50
#define	FILE_NAME_MAX			50
#define	FILE_REGEX_MAX			8192
#define	FILE_ENCODING_MAX		(64 * 1024)
#define 		EVENT_HAD_ERR		0x01
#define FILE_BYTES_MAX (1024 * 1024)
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
#define FILE_BADSIZE CAST(size_t, ~0ul)

#define num_mask _u._mask
#define str_range _u._s._count
#define str_flags _u._s._flags
#define				COND_NONE	0
#define				COND_IF		1
#define				COND_ELIF	2
#define				COND_ELSE	3
#define MAXstring 128		/* max len of "string" types */
#define MAXDESC	64		/* max len of text description/MIME type */
#define MAXMIME	80		/* max len of text MIME type */
#define MAGIC_SETS	2
#define mode_t unsigned short

#define	MAGIC_NONE		0x0000000 /* No flags */
#define	MAGIC_DEBUG		0x0000001 /* Turn on debugging */
#define	MAGIC_SYMLINK		0x0000002 /* Follow symlinks */
#define	MAGIC_COMPRESS		0x0000004 /* Check inside compressed files */
#define	MAGIC_DEVICES		0x0000008 /* Look at the contents of devices */
#define	MAGIC_MIME_TYPE		0x0000010 /* Return the MIME type */
#define	MAGIC_CONTINUE		0x0000020 /* Return all matches */
#define	MAGIC_CHECK		0x0000040 /* Print warnings to stderr */
#define	MAGIC_PRESERVE_ATIME	0x0000080 /* Restore access time on exit */
#define	MAGIC_RAW		0x0000100 /* Don't convert unprintable chars */
#define	MAGIC_ERROR		0x0000200 /* Handle ENOENT etc as real errors */
#define	MAGIC_MIME_ENCODING	0x0000400 /* Return the MIME encoding */
#define MAGIC_MIME		(MAGIC_MIME_TYPE|MAGIC_MIME_ENCODING)
#define	MAGIC_APPLE		0x0000800 /* Return the Apple creator/type */
#define	MAGIC_EXTENSION		0x1000000 /* Return a /-separated list of * extensions */
#define MAGIC_COMPRESS_TRANSP	0x2000000 /* Check inside compressed files * but not report compression */
#define MAGIC_NODESC		(MAGIC_EXTENSION|MAGIC_MIME|MAGIC_APPLE)

#define	MAGIC_NO_CHECK_COMPRESS	0x0001000 /* Don't check for compressed files */
#define	MAGIC_NO_CHECK_TAR	0x0002000 /* Don't check for tar files */
#define	MAGIC_NO_CHECK_SOFT	0x0004000 /* Don't check magic entries */
#define	MAGIC_NO_CHECK_APPTYPE	0x0008000 /* Don't check application type */
#define	MAGIC_NO_CHECK_ELF	0x0010000 /* Don't check for elf details */
#define	MAGIC_NO_CHECK_TEXT	0x0020000 /* Don't check for text files */
#define	MAGIC_NO_CHECK_CDF	0x0040000 /* Don't check for cdf files */
#define MAGIC_NO_CHECK_CSV	0x0080000 /* Don't check for CSV files */
#define	MAGIC_NO_CHECK_TOKENS	0x0100000 /* Don't check tokens */
#define MAGIC_NO_CHECK_ENCODING 0x0200000 /* Don't check text encodings */
#define MAGIC_NO_CHECK_JSON	0x0400000 /* Don't check for JSON files */

/* No built-in tests; only consult the magic file */
#define MAGIC_NO_CHECK_BUILTIN	( \
	MAGIC_NO_CHECK_COMPRESS	| \
	MAGIC_NO_CHECK_TAR	| \
/*	MAGIC_NO_CHECK_SOFT	| */ \
	MAGIC_NO_CHECK_APPTYPE	| \
	MAGIC_NO_CHECK_ELF	| \
	MAGIC_NO_CHECK_TEXT	| \
	MAGIC_NO_CHECK_CSV	| \
	MAGIC_NO_CHECK_CDF	| \
	MAGIC_NO_CHECK_TOKENS	| \
	MAGIC_NO_CHECK_ENCODING	| \
	MAGIC_NO_CHECK_JSON	| \
	0			  \
)

#define	FILE_LOAD	0
#define FILE_CHECK	1
#define FILE_COMPILE	2
#define FILE_LIST	3
#define MAP_TYPE_USER	0
#define MAP_TYPE_MALLOC	1
#define MAP_TYPE_MMAP	2

#define CAST(T, b)	((T)(b))
#define RCAST(T, b)	((T)(uintptr_t)(b))
#define CCAST(T, b)	((T)(uintptr_t)(b))
#define FILE_GUID_SIZE	sizeof("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX")
#define MAGICNO		0xF11E041C
#define VERSIONNO	16

struct type_tbl_s {
	const char name[16];
	const size_t len;
	const int type;
	const int format;
};

typedef struct {
	const char *pat;

	locale_t old_lc_ctype;
	locale_t c_lc_ctype;

	int rc;
	regex_t rx;
} file_regex_t;

typedef unsigned long file_unichar_t;

struct guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
};

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
#define XXD(s)		s, (sizeof(s) - 1)
#define XXD_NULL	"", 0

static const struct type_tbl_s type_tbl[] = {

	{ XXD("invalid"),	FILE_INVALID,		FILE_FMT_NONE },
	{ XXD("byte"),		FILE_BYTE,		FILE_FMT_NUM },
	{ XXD("short"),		FILE_SHORT,		FILE_FMT_NUM },
	{ XXD("default"),	FILE_DEFAULT,		FILE_FMT_NONE },
	{ XXD("long"),		FILE_LONG,		FILE_FMT_NUM },
	{ XXD("string"),		FILE_STRING,		FILE_FMT_STR },
	{ XXD("date"),		FILE_DATE,		FILE_FMT_STR },
	{ XXD("beshort"),	FILE_BESHORT,		FILE_FMT_NUM },
	{ XXD("belong"),		FILE_BELONG,		FILE_FMT_NUM },
	{ XXD("bedate"),		FILE_BEDATE,		FILE_FMT_STR },
	{ XXD("leshort"),	FILE_LESHORT,		FILE_FMT_NUM },
	{ XXD("lelong"),		FILE_LELONG,		FILE_FMT_NUM },
	{ XXD("ledate"),		FILE_LEDATE,		FILE_FMT_STR },
	{ XXD("pstring"),	FILE_PSTRING,		FILE_FMT_STR },
	{ XXD("ldate"),		FILE_LDATE,		FILE_FMT_STR },
	{ XXD("beldate"),	FILE_BELDATE,		FILE_FMT_STR },
	{ XXD("leldate"),	FILE_LELDATE,		FILE_FMT_STR },
	{ XXD("regex"),		FILE_REGEX,		FILE_FMT_STR },
	{ XXD("bestring16"),	FILE_BESTRING16,	FILE_FMT_STR },
	{ XXD("lestring16"),	FILE_LESTRING16,	FILE_FMT_STR },
	{ XXD("search"),		FILE_SEARCH,		FILE_FMT_STR },
	{ XXD("medate"),		FILE_MEDATE,		FILE_FMT_STR },
	{ XXD("meldate"),	FILE_MELDATE,		FILE_FMT_STR },
	{ XXD("melong"),		FILE_MELONG,		FILE_FMT_NUM },
	{ XXD("quad"),		FILE_QUAD,		FILE_FMT_QUAD },
	{ XXD("lequad"),		FILE_LEQUAD,		FILE_FMT_QUAD },
	{ XXD("bequad"),		FILE_BEQUAD,		FILE_FMT_QUAD },
	{ XXD("qdate"),		FILE_QDATE,		FILE_FMT_STR },
	{ XXD("leqdate"),	FILE_LEQDATE,		FILE_FMT_STR },
	{ XXD("beqdate"),	FILE_BEQDATE,		FILE_FMT_STR },
	{ XXD("qldate"),		FILE_QLDATE,		FILE_FMT_STR },
	{ XXD("leqldate"),	FILE_LEQLDATE,		FILE_FMT_STR },
	{ XXD("beqldate"),	FILE_BEQLDATE,		FILE_FMT_STR },
	{ XXD("float"),		FILE_FLOAT,		FILE_FMT_FLOAT },
	{ XXD("befloat"),	FILE_BEFLOAT,		FILE_FMT_FLOAT },
	{ XXD("lefloat"),	FILE_LEFLOAT,		FILE_FMT_FLOAT },
	{ XXD("double"),		FILE_DOUBLE,		FILE_FMT_DOUBLE },
	{ XXD("bedouble"),	FILE_BEDOUBLE,		FILE_FMT_DOUBLE },
	{ XXD("ledouble"),	FILE_LEDOUBLE,		FILE_FMT_DOUBLE },
	{ XXD("leid3"),		FILE_LEID3,		FILE_FMT_NUM },
	{ XXD("beid3"),		FILE_BEID3,		FILE_FMT_NUM },
	{ XXD("indirect"),	FILE_INDIRECT,		FILE_FMT_NUM },
	{ XXD("qwdate"),		FILE_QWDATE,		FILE_FMT_STR },
	{ XXD("leqwdate"),	FILE_LEQWDATE,		FILE_FMT_STR },
	{ XXD("beqwdate"),	FILE_BEQWDATE,		FILE_FMT_STR },
	{ XXD("name"),		FILE_NAME,		FILE_FMT_NONE },
	{ XXD("use"),		FILE_USE,		FILE_FMT_NONE },
	{ XXD("clear"),		FILE_CLEAR,		FILE_FMT_NONE },
	{ XXD("der"),		FILE_DER,		FILE_FMT_STR },
	{ XXD("guid"),		FILE_GUID,		FILE_FMT_STR },
	{ XXD("offset"),		FILE_OFFSET,		FILE_FMT_QUAD },
	{ XXD_NULL,		FILE_INVALID,		FILE_FMT_NONE },
};

#define PATHSEP	':'
#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define FILE_MAGICSIZE	376

const char *file_names[FILE_NAMES_SIZE];
int file_formats[FILE_NAMES_SIZE];

struct file_search{
	const char *s;		/* start of search in original source */
	size_t s_len;		/* length of search region */
	size_t offset;		/* starting offset in source: XXX - should this be off_t? */
	size_t rm_len;		/* match length */
};

struct file_s{
	uint32_t _count;	/* repeat/line count */
	uint32_t _flags;	/* modifier flags */
};

union file_u {
	uint64_t _mask; /* for use with numeric and date types */
	struct file_s _s;		/* for use with string types */
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
	uint16_t cont_level;	/* level of ">" */
	uint8_t flag;
	uint8_t factor;
	uint8_t reln;		/* relation (0=eq, '>'=gt, etc) */
	uint8_t vallen;		/* length of string value, if any */
	uint8_t type;		/* comparison type (FILE_*) */
	uint8_t in_type;	/* type of indirection */
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
	union file_u _u;

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

struct level_info {
	int32_t off;
	int got_match;
	int last_match;
	int last_cond;	/* used for error checking by parse() */
};

struct mlist {
	struct magic *magic;		/* array of magic entries */
	uint32_t nmagic;		/* number of entries in array */
	void *map;			/* internal resources used by entry */
	struct mlist *next, *prev;
};

struct cont {
	size_t len;
	struct level_info *li;
};

struct out {
	char *buf;		/* Accumulation buffer */
	size_t blen;		/* Length of buffer */
	char *pbuf; 	/* Printable buffer */
};

struct magic_map {
	void *p;
	size_t len;
	int type;
	struct magic *magic[MAGIC_SETS];
	uint32_t nmagic[MAGIC_SETS];
};

struct magic_set {
	struct mlist *mlist[MAGIC_SETS];	/* list of regular entries */
	struct cont c;
	struct out o;
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
	struct file_search search;

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

struct magic_entry {
	struct magic *mp;
	uint32_t cont_count;
	uint32_t max_count;
};

struct magic_entry_set {
	struct magic_entry *me;
	uint32_t count;
	uint32_t max;
};

struct magic_set *file_ms_alloc(int flags);
int magic_compile(struct magic_set *ms, const char *magicfile, char *out_magic);

void magic_close(struct magic_set *ms);

#endif

