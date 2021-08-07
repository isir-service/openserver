
#include "file.h"


#include <string.h>
#include "magic.h"
#include <sys/types.h>



#define DPRINTF(fmt, ...)

/*
 * if CSV_LINES == 0:
 *	check all the lines in the buffer
 * otherwise:
 *	check only up-to the number of lines specified
 *
 * the last line count is always ignored if it does not end in CRLF
 */
#define CSV_LINES 10

static int csv_parse(const unsigned char *, const unsigned char *);

static const unsigned char *
eatquote(const unsigned char *uc, const unsigned char *ue)
{
	int quote = 0;

	while (uc < ue) {
		unsigned char c = *uc++;
		if (c != '"') {
			// We already got one, done.
			if (quote) {
				return --uc;
			}
			continue;
		}
		if (quote) {
			// quote-quote escapes
			quote = 0;
			continue;
		}
		// first quote
		quote = 1;
	}
	return ue;
}

static int
csv_parse(const unsigned char *uc, const unsigned char *ue)
{
	size_t nf = 0, tf = 0, nl = 0;

	while (uc < ue) {
		switch (*uc++) {
		case '"':
			// Eat until the matching quote
			uc = eatquote(uc, ue);
			break;
		case ',':
			nf++;
			break;
		case '\n':
			DPRINTF("%zu %zu %zu\n", nl, nf, tf);
			nl++;
			if (nl == CSV_LINES)
				return tf != 0 && tf == nf;

			if (tf == 0) {
				// First time and no fields, give up
				if (nf == 0) 
					return 0;
				// First time, set the number of fields
				tf = nf;
			} else if (tf != nf) {
				// Field number mismatch, we are done.
				return 0;
			}
			nf = 0;
			break;
		default:
			break;
		}
	}
	return tf && nl > 2;
}

int
file_is_csv(struct magic_set *ms, const struct buffer *b, int looks_text)
{
	const unsigned char *uc = CAST(const unsigned char *, b->fbuf);
	const unsigned char *ue = uc + b->flen;
	int mime = ms->flags & MAGIC_MIME;

	if (!looks_text)
		return 0;

	if ((ms->flags & (MAGIC_APPLE|MAGIC_EXTENSION)) != 0)
		return 0;

	if (!csv_parse(uc, ue))
		return 0;

	if (mime == MAGIC_MIME_ENCODING)
		return 1;

	if (mime) {
		if (file_printf(ms, "text/csv") == -1)
			return -1;
		return 1;
	}

	if (file_printf(ms, "CSV text") == -1)
		return -1;

	return 1;
}

