
#include "file.h"


#include <string.h>
#include "magic.h"


#include <stdio.h>

#define DPRINTF(a, b, c)	do { } while (/*CONSTCOND*/0)

#define JSON_ARRAY	0
#define JSON_CONSTANT	1
#define JSON_NUMBER	2
#define JSON_OBJECT	3
#define JSON_STRING	4
#define JSON_ARRAYN	5
#define JSON_MAX	6

/*
 * if JSON_COUNT != 0:
 *	count all the objects, require that we have the whole data file
 * otherwise:
 *	stop if we find an object or an array
 */
#define JSON_COUNT 0

static int json_parse(const unsigned char **, const unsigned char *, size_t *,
	size_t);

static int
json_isspace(const unsigned char uc)
{
	switch (uc) {
	case ' ':
	case '\n':
	case '\r':
	case '\t':
		return 1;
	default:
		return 0;
	}
}

static int
json_isdigit(unsigned char uc)
{
	switch (uc) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return 1;
	default:
		return 0;
	}
}

static int
json_isxdigit(unsigned char uc)
{
	if (json_isdigit(uc))
		return 1;
	switch (uc) {
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return 1;
	default:
		return 0;
	}
}

static const unsigned char *
json_skip_space(const unsigned char *uc, const unsigned char *ue)
{
	while (uc < ue && json_isspace(*uc))
		uc++;
	return uc;
}

static int
json_parse_string(const unsigned char **ucp, const unsigned char *ue)
{
	const unsigned char *uc = *ucp;
	size_t i;

	DPRINTF("Parse string: ", uc, *ucp);
	while (uc < ue) {
		switch (*uc++) {
		case '\0':
			goto out;
		case '\\':
			if (uc == ue)
				goto out;
			switch (*uc++) {
			case '\0':
				goto out;
			case '"':
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				continue;
			case 'u':
				if (ue - uc < 4) {
					uc = ue;
					goto out;
				}
				for (i = 0; i < 4; i++)
					if (!json_isxdigit(*uc++))
						goto out;
				continue;
			default:
				goto out;
			}
		case '"':
			*ucp = uc;
			DPRINTF("Good string: ", uc, *ucp);
			return 1;
		default:
			continue;
		}
	}
out:
	DPRINTF("Bad string: ", uc, *ucp);
	*ucp = uc;
	return 0;
}

static int
json_parse_array(const unsigned char **ucp, const unsigned char *ue,
	size_t *st, size_t lvl)
{
	const unsigned char *uc = *ucp;

	DPRINTF("Parse array: ", uc, *ucp);
	while (uc < ue) {
		if (*uc == ']')
			goto done;
		if (!json_parse(&uc, ue, st, lvl + 1))
			goto out;
		if (uc == ue)
			goto out;
		switch (*uc) {
		case ',':
			uc++;
			continue;
		case ']':
		done:
			st[JSON_ARRAYN]++;
			*ucp = uc + 1;
			DPRINTF("Good array: ", uc, *ucp);
			return 1;
		default:
			goto out;
		}
	}
out:
	DPRINTF("Bad array: ", uc,  *ucp);
	*ucp = uc;
	return 0;
}

static int
json_parse_object(const unsigned char **ucp, const unsigned char *ue,
	size_t *st, size_t lvl)
{
	const unsigned char *uc = *ucp;
	DPRINTF("Parse object: ", uc, *ucp);
	while (uc < ue) {
		uc = json_skip_space(uc, ue);
		if (uc == ue)
			goto out;
		if (*uc == '}') {
			uc++;
			goto done;
		}
		if (*uc++ != '"') {
			DPRINTF("not string", uc, *ucp);
			goto out;
		}
		DPRINTF("next field", uc, *ucp);
		if (!json_parse_string(&uc, ue)) {
			DPRINTF("not string", uc, *ucp);
			goto out;
		}
		uc = json_skip_space(uc, ue);
		if (uc == ue)
			goto out;
		if (*uc++ != ':') {
			DPRINTF("not colon", uc, *ucp);
			goto out;
		}
		if (!json_parse(&uc, ue, st, lvl + 1)) {
			DPRINTF("not json", uc, *ucp);
			goto out;
		}
		if (uc == ue)
			goto out;
		switch (*uc++) {
		case ',':
			continue;
		case '}': /* { */
		done:
			*ucp = uc;
			DPRINTF("Good object: ", uc, *ucp);
			return 1;
		default:
			*ucp = uc - 1;
			DPRINTF("not more", uc, *ucp);
			goto out;
		}
	}
out:
	DPRINTF("Bad object: ", uc, *ucp);
	*ucp = uc;
	return 0;
}

static int
json_parse_number(const unsigned char **ucp, const unsigned char *ue)
{
	const unsigned char *uc = *ucp;
	int got = 0;

	DPRINTF("Parse number: ", uc, *ucp);
	if (uc == ue)
		return 0;
	if (*uc == '-')
		uc++;

	for (; uc < ue; uc++) {
		if (!json_isdigit(*uc))
			break;
		got = 1;
	}
	if (uc == ue)
		goto out;
	if (*uc == '.')
		uc++;
	for (; uc < ue; uc++) {
		if (!json_isdigit(*uc))
			break;
		got = 1;
	}
	if (uc == ue)
		goto out;
	if (got && (*uc == 'e' || *uc == 'E')) {
		uc++;
		got = 0;
		if (uc == ue)
			goto out;
		if (*uc == '+' || *uc == '-')
			uc++;
		for (; uc < ue; uc++) {
			if (!json_isdigit(*uc))
				break;
			got = 1;
		}
	}
out:
	if (!got)
		DPRINTF("Bad number: ", uc, *ucp);
	else
		DPRINTF("Good number: ", uc, *ucp);
	*ucp = uc;
	return got;
}

static int
json_parse_const(const unsigned char **ucp, const unsigned char *ue,
    const char *str, size_t len)
{
	const unsigned char *uc = *ucp;

	DPRINTF("Parse const: ", uc, *ucp);
	for (len--; uc < ue && --len;) {
		if (*uc++ == *++str)
			continue;
	}
	if (len)
		DPRINTF("Bad const: ", uc, *ucp);
	*ucp = uc;
	return len == 0;
}

static int
json_parse(const unsigned char **ucp, const unsigned char *ue,
    size_t *st, size_t lvl)
{
	const unsigned char *uc;
	int rv = 0;
	int t;

	uc = json_skip_space(*ucp, ue);
	if (uc == ue)
		goto out;

	// Avoid recursion
	if (lvl > 20)
		return 0;
	/* bail quickly if not counting */
	if (lvl > 1 && (st[JSON_OBJECT] || st[JSON_ARRAYN]))
		return 1;

	DPRINTF("Parse general: ", uc, *ucp);
	switch (*uc++) {
	case '"':
		rv = json_parse_string(&uc, ue);
		t = JSON_STRING;
		break;
	case '[':
		rv = json_parse_array(&uc, ue, st, lvl + 1);
		t = JSON_ARRAY;
		break;
	case '{': /* '}' */
		rv = json_parse_object(&uc, ue, st, lvl + 1);
		t = JSON_OBJECT;
		break;
	case 't':
		rv = json_parse_const(&uc, ue, "true", sizeof("true"));
		t = JSON_CONSTANT;
		break;
	case 'f':
		rv = json_parse_const(&uc, ue, "false", sizeof("false"));
		t = JSON_CONSTANT;
		break;
	case 'n':
		rv = json_parse_const(&uc, ue, "null", sizeof("null"));
		t = JSON_CONSTANT;
		break;
	default:
		--uc;
		rv = json_parse_number(&uc, ue);
		t = JSON_NUMBER;
		break;
	}
	if (rv)
		st[t]++;
	uc = json_skip_space(uc, ue);
out:
	*ucp = uc;
	DPRINTF("End general: ", uc, *ucp);
	if (lvl == 0)
		return rv && (st[JSON_ARRAYN] || st[JSON_OBJECT]);
	return rv;
}


int file_is_json(struct magic_set *ms, const struct buffer *b)
{
	const unsigned char *uc = CAST(const unsigned char *, b->fbuf);
	const unsigned char *ue = uc + b->flen;
	size_t st[JSON_MAX];
	int mime = ms->flags & MAGIC_MIME;


	if ((ms->flags & (MAGIC_APPLE|MAGIC_EXTENSION)) != 0)
		return 0;

	memset(st, 0, sizeof(st));

	if (!json_parse(&uc, ue, st, 0))
		return 0;

	if (mime == MAGIC_MIME_ENCODING)
		return 1;
	if (mime) {
		if (file_printf(ms, "application/json") == -1)
			return -1;
		return 1;
	}
	if (file_printf(ms, "JSON data") == -1)
		return -1;
#define P(n) st[n], st[n] > 1 ? "s" : ""
	if (file_printf(ms, " (%" SIZE_T_FORMAT "u object%s, %" SIZE_T_FORMAT
	    "u array%s, %" SIZE_T_FORMAT "u string%s, %" SIZE_T_FORMAT
	    "u constant%s, %" SIZE_T_FORMAT "u number%s, %" SIZE_T_FORMAT
	    "u >1array%s)",
	    P(JSON_OBJECT), P(JSON_ARRAY), P(JSON_STRING), P(JSON_CONSTANT),
	    P(JSON_NUMBER), P(JSON_ARRAYN))
	    == -1)
		return -1;
	return 1;
}
