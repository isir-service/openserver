
#include "file.h"

#include <assert.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "cdf.h"

#define EFTYPE EINVAL

#define SIZE_T_MAX CAST(size_t, ~0ULL)

#define DPRINTF(a)

static union {
	char s[4];
	uint32_t u;
} cdf_bo;

#define NEED_SWAP	(cdf_bo.u == CAST(uint32_t, 0x01020304))

#define CDF_TOLE8(x)	\
    (CAST(uint64_t, NEED_SWAP ? _cdf_tole8(x) : CAST(uint64_t, x)))
#define CDF_TOLE4(x)	\
    (CAST(uint32_t, NEED_SWAP ? _cdf_tole4(x) : CAST(uint32_t, x)))
#define CDF_TOLE2(x)	\
    (CAST(uint16_t, NEED_SWAP ? _cdf_tole2(x) : CAST(uint16_t, x)))
#define CDF_TOLE(x)	(/*CONSTCOND*/sizeof(x) == 2 ? \
			    CDF_TOLE2(CAST(uint16_t, x)) : \
			(/*CONSTCOND*/sizeof(x) == 4 ? \
			    CDF_TOLE4(CAST(uint32_t, x)) : \
			    CDF_TOLE8(CAST(uint64_t, x))))
#define CDF_GETUINT32(x, y)	cdf_getuint32(x, y)

#define CDF_MALLOC(n) cdf_malloc(__FILE__, __LINE__, (n))
#define CDF_REALLOC(p, n) cdf_realloc(__FILE__, __LINE__, (p), (n))
#define CDF_CALLOC(n, u) cdf_calloc(__FILE__, __LINE__, (n), (u))
#define CDF_UNPACK(a)	\
    (void)memcpy(&(a), &buf[len], sizeof(a)), len += sizeof(a)
#define CDF_UNPACKA(a)	\
    (void)memcpy((a), &buf[len], sizeof(a)), len += sizeof(a)
#define CDF_SHLEN_LIMIT (UINT32_MAX / 64)
#define CDF_PROP_LIMIT (UINT32_MAX / (64 * sizeof(cdf_property_info_t)))

struct _vn{
	uint32_t v;
	const char *n;
} ;

struct _vn vn[] = {
	{ CDF_PROPERTY_CODE_PAGE, "Code page" },
	{ CDF_PROPERTY_TITLE, "Title" },
	{ CDF_PROPERTY_SUBJECT, "Subject" },
	{ CDF_PROPERTY_AUTHOR, "Author" },
	{ CDF_PROPERTY_KEYWORDS, "Keywords" },
	{ CDF_PROPERTY_COMMENTS, "Comments" },
	{ CDF_PROPERTY_TEMPLATE, "Template" },
	{ CDF_PROPERTY_LAST_SAVED_BY, "Last Saved By" },
	{ CDF_PROPERTY_REVISION_NUMBER, "Revision Number" },
	{ CDF_PROPERTY_TOTAL_EDITING_TIME, "Total Editing Time" },
	{ CDF_PROPERTY_LAST_PRINTED, "Last Printed" },
	{ CDF_PROPERTY_CREATE_TIME, "Create Time/Date" },
	{ CDF_PROPERTY_LAST_SAVED_TIME, "Last Saved Time/Date" },
	{ CDF_PROPERTY_NUMBER_OF_PAGES, "Number of Pages" },
	{ CDF_PROPERTY_NUMBER_OF_WORDS, "Number of Words" },
	{ CDF_PROPERTY_NUMBER_OF_CHARACTERS, "Number of Characters" },
	{ CDF_PROPERTY_THUMBNAIL, "Thumbnail" },
	{ CDF_PROPERTY_NAME_OF_APPLICATION, "Name of Creating Application" },
	{ CDF_PROPERTY_SECURITY, "Security" },
	{ CDF_PROPERTY_LOCALE_ID, "Locale ID" },
};


/*ARGSUSED*/
static void * cdf_malloc(const char *file __attribute__((__unused__)),
    size_t line __attribute__((__unused__)), size_t n)
{
	DPRINTF(("%s,%" SIZE_T_FORMAT "u: %s %" SIZE_T_FORMAT "u\n",
	    file, line, __func__, n));
	return malloc(n);
}

/*ARGSUSED*/
static void * cdf_realloc(const char *file __attribute__((__unused__)),
    size_t line __attribute__((__unused__)), void *p, size_t n)
{
	DPRINTF(("%s,%" SIZE_T_FORMAT "u: %s %" SIZE_T_FORMAT "u\n",
	    file, line, __func__, n));
	return realloc(p, n);
}

/*ARGSUSED*/
static void * cdf_calloc(const char *file __attribute__((__unused__)),
    size_t line __attribute__((__unused__)), size_t n, size_t u)
{
	DPRINTF(("%s,%" SIZE_T_FORMAT "u: %s %" SIZE_T_FORMAT "u %"
	    SIZE_T_FORMAT "u\n", file, line, __func__, n, u));
	return calloc(n, u);
}

/*
 * swap a short
 */
static uint16_t _cdf_tole2(uint16_t sv)
{
	uint16_t rv;
	uint8_t *s = RCAST(uint8_t *, RCAST(void *, &sv));
	uint8_t *d = RCAST(uint8_t *, RCAST(void *, &rv));
	d[0] = s[1];
	d[1] = s[0];
	return rv;
}

/*
 * swap an int
 */
static uint32_t _cdf_tole4(uint32_t sv)
{
	uint32_t rv;
	uint8_t *s = RCAST(uint8_t *, RCAST(void *, &sv));
	uint8_t *d = RCAST(uint8_t *, RCAST(void *, &rv));
	d[0] = s[3];
	d[1] = s[2];
	d[2] = s[1];
	d[3] = s[0];
	return rv;
}

/*
 * swap a quad
 */
static uint64_t _cdf_tole8(uint64_t sv)
{
	uint64_t rv;
	uint8_t *s = RCAST(uint8_t *, RCAST(void *, &sv));
	uint8_t *d = RCAST(uint8_t *, RCAST(void *, &rv));
	d[0] = s[7];
	d[1] = s[6];
	d[2] = s[5];
	d[3] = s[4];
	d[4] = s[3];
	d[5] = s[2];
	d[6] = s[1];
	d[7] = s[0];
	return rv;
}

/*
 * grab a uint32_t from a possibly unaligned address, and return it in
 * the native host order.
 */
static uint32_t cdf_getuint32(const uint8_t *p, size_t offs)
{
	uint32_t rv;
	(void)memcpy(&rv, p + offs * sizeof(uint32_t), sizeof(rv));
	return CDF_TOLE4(rv);
}

uint16_t cdf_tole2(uint16_t sv)
{
	return CDF_TOLE2(sv);
}

uint32_t cdf_tole4(uint32_t sv)
{
	return CDF_TOLE4(sv);
}

uint64_t cdf_tole8(uint64_t sv)
{
	return CDF_TOLE8(sv);
}

void
cdf_swap_header(cdf_header_t *h)
{
	size_t i;

	h->h_magic = CDF_TOLE8(h->h_magic);
	h->h_uuid[0] = CDF_TOLE8(h->h_uuid[0]);
	h->h_uuid[1] = CDF_TOLE8(h->h_uuid[1]);
	h->h_revision = CDF_TOLE2(h->h_revision);
	h->h_version = CDF_TOLE2(h->h_version);
	h->h_byte_order = CDF_TOLE2(h->h_byte_order);
	h->h_sec_size_p2 = CDF_TOLE2(h->h_sec_size_p2);
	h->h_short_sec_size_p2 = CDF_TOLE2(h->h_short_sec_size_p2);
	h->h_num_sectors_in_sat = CDF_TOLE4(h->h_num_sectors_in_sat);
	h->h_secid_first_directory = CDF_TOLE4(h->h_secid_first_directory);
	h->h_min_size_standard_stream =
	    CDF_TOLE4(h->h_min_size_standard_stream);
	h->h_secid_first_sector_in_short_sat =
	    CDF_TOLE4(CAST(uint32_t, h->h_secid_first_sector_in_short_sat));
	h->h_num_sectors_in_short_sat =
	    CDF_TOLE4(h->h_num_sectors_in_short_sat);
	h->h_secid_first_sector_in_master_sat =
	    CDF_TOLE4(CAST(uint32_t, h->h_secid_first_sector_in_master_sat));
	h->h_num_sectors_in_master_sat =
	    CDF_TOLE4(h->h_num_sectors_in_master_sat);
	for (i = 0; i < __arraycount(h->h_master_sat); i++) {
		h->h_master_sat[i] =
		    CDF_TOLE4(CAST(uint32_t, h->h_master_sat[i]));
	}
}

void
cdf_unpack_header(cdf_header_t *h, char *buf)
{
	size_t i;
	size_t len = 0;

	CDF_UNPACK(h->h_magic);
	CDF_UNPACKA(h->h_uuid);
	CDF_UNPACK(h->h_revision);
	CDF_UNPACK(h->h_version);
	CDF_UNPACK(h->h_byte_order);
	CDF_UNPACK(h->h_sec_size_p2);
	CDF_UNPACK(h->h_short_sec_size_p2);
	CDF_UNPACKA(h->h_unused0);
	CDF_UNPACK(h->h_num_sectors_in_sat);
	CDF_UNPACK(h->h_secid_first_directory);
	CDF_UNPACKA(h->h_unused1);
	CDF_UNPACK(h->h_min_size_standard_stream);
	CDF_UNPACK(h->h_secid_first_sector_in_short_sat);
	CDF_UNPACK(h->h_num_sectors_in_short_sat);
	CDF_UNPACK(h->h_secid_first_sector_in_master_sat);
	CDF_UNPACK(h->h_num_sectors_in_master_sat);
	for (i = 0; i < __arraycount(h->h_master_sat); i++)
		CDF_UNPACK(h->h_master_sat[i]);
}

void
cdf_swap_dir(cdf_directory_t *d)
{
	d->d_namelen = CDF_TOLE2(d->d_namelen);
	d->d_left_child = CDF_TOLE4(CAST(uint32_t, d->d_left_child));
	d->d_right_child = CDF_TOLE4(CAST(uint32_t, d->d_right_child));
	d->d_storage = CDF_TOLE4(CAST(uint32_t, d->d_storage));
	d->d_storage_uuid[0] = CDF_TOLE8(d->d_storage_uuid[0]);
	d->d_storage_uuid[1] = CDF_TOLE8(d->d_storage_uuid[1]);
	d->d_flags = CDF_TOLE4(d->d_flags);
	d->d_created = CDF_TOLE8(CAST(uint64_t, d->d_created));
	d->d_modified = CDF_TOLE8(CAST(uint64_t, d->d_modified));
	d->d_stream_first_sector = CDF_TOLE4(
	    CAST(uint32_t, d->d_stream_first_sector));
	d->d_size = CDF_TOLE4(d->d_size);
}

void
cdf_swap_class(cdf_classid_t *d)
{
	d->cl_dword = CDF_TOLE4(d->cl_dword);
	d->cl_word[0] = CDF_TOLE2(d->cl_word[0]);
	d->cl_word[1] = CDF_TOLE2(d->cl_word[1]);
}

void
cdf_unpack_dir(cdf_directory_t *d, char *buf)
{
	size_t len = 0;

	CDF_UNPACKA(d->d_name);
	CDF_UNPACK(d->d_namelen);
	CDF_UNPACK(d->d_type);
	CDF_UNPACK(d->d_color);
	CDF_UNPACK(d->d_left_child);
	CDF_UNPACK(d->d_right_child);
	CDF_UNPACK(d->d_storage);
	CDF_UNPACKA(d->d_storage_uuid);
	CDF_UNPACK(d->d_flags);
	CDF_UNPACK(d->d_created);
	CDF_UNPACK(d->d_modified);
	CDF_UNPACK(d->d_stream_first_sector);
	CDF_UNPACK(d->d_size);
	CDF_UNPACK(d->d_unused0);
}

int
cdf_zero_stream(cdf_stream_t *scn)
{
	scn->sst_len = 0;
	scn->sst_dirlen = 0;
	scn->sst_ss = 0;
	free(scn->sst_tab);
	scn->sst_tab = NULL;
	return -1;
}

static size_t
cdf_check_stream(const cdf_stream_t *sst, const cdf_header_t *h)
{
	size_t ss = sst->sst_dirlen < h->h_min_size_standard_stream ?
	    CDF_SHORT_SEC_SIZE(h) : CDF_SEC_SIZE(h);
	assert(ss == sst->sst_ss);
	return sst->sst_ss;
}

static int
cdf_check_stream_offset(const cdf_stream_t *sst, const cdf_header_t *h,
    const void *p, size_t tail, int line)
{
	const char *b = RCAST(const char *, sst->sst_tab);
	const char *e = RCAST(const char *, p) + tail;
	size_t ss = cdf_check_stream(sst, h);
	/*LINTED*/(void)&line;
	if (e >= b && CAST(size_t, e - b) <= ss * sst->sst_len)
		return 0;
	DPRINTF(("%d: offset begin %p < end %p || %" SIZE_T_FORMAT "u"
	    " > %" SIZE_T_FORMAT "u [%" SIZE_T_FORMAT "u %"
	    SIZE_T_FORMAT "u]\n", line, b, e, (size_t)(e - b),
	    ss * sst->sst_len, ss, sst->sst_len));
	errno = EFTYPE;
	return -1;
}

static ssize_t
cdf_read(const cdf_info_t *info, off_t off, void *buf, size_t len)
{
	size_t siz = CAST(size_t, off + len);

	if (CAST(off_t, off + len) != CAST(off_t, siz))
		goto out;

	if (info->i_buf != NULL && info->i_len >= siz) {
		(void)memcpy(buf, &info->i_buf[off], len);
		return CAST(ssize_t, len);
	}

	if (info->i_fd == -1)
		goto out;

	if (pread(info->i_fd, buf, len, off) != CAST(ssize_t, len))
		return -1;

	return CAST(ssize_t, len);
out:
	errno = EINVAL;
	return -1;
}

int
cdf_read_header(const cdf_info_t *info, cdf_header_t *h)
{
	char buf[512];

	(void)memcpy(cdf_bo.s, "\01\02\03\04", 4);
	if (cdf_read(info, CAST(off_t, 0), buf, sizeof(buf)) == -1)
		return -1;
	cdf_unpack_header(h, buf);
	cdf_swap_header(h);
	if (h->h_magic != CDF_MAGIC) {
		DPRINTF(("Bad magic %#" INT64_T_FORMAT "x != %#"
		    INT64_T_FORMAT "x\n",
		    (unsigned long long)h->h_magic,
		    (unsigned long long)CDF_MAGIC));
		goto out;
	}
	if (h->h_sec_size_p2 > 20) {
		DPRINTF(("Bad sector size %hu\n", h->h_sec_size_p2));
		goto out;
	}
	if (h->h_short_sec_size_p2 > 20) {
		DPRINTF(("Bad short sector size %hu\n",
		    h->h_short_sec_size_p2));
		goto out;
	}
	return 0;
out:
	errno = EFTYPE;
	return -1;
}


ssize_t
cdf_read_sector(const cdf_info_t *info, void *buf, size_t offs, size_t len,
    const cdf_header_t *h, cdf_secid_t id)
{
	size_t ss = CDF_SEC_SIZE(h);
	size_t pos;

	if (SIZE_T_MAX / ss < CAST(size_t, id))
		return -1;

	pos = CDF_SEC_POS(h, id);
	assert(ss == len);
	return cdf_read(info, CAST(off_t, pos), RCAST(char *, buf) + offs, len);
}

ssize_t
cdf_read_short_sector(const cdf_stream_t *sst, void *buf, size_t offs,
    size_t len, const cdf_header_t *h, cdf_secid_t id)
{
	size_t ss = CDF_SHORT_SEC_SIZE(h);
	size_t pos;

	if (SIZE_T_MAX / ss < CAST(size_t, id))
		return -1;

	pos = CDF_SHORT_SEC_POS(h, id);
	assert(ss == len);
	if (pos + len > CDF_SEC_SIZE(h) * sst->sst_len) {
		DPRINTF(("Out of bounds read %" SIZE_T_FORMAT "u > %"
		    SIZE_T_FORMAT "u\n",
		    pos + len, CDF_SEC_SIZE(h) * sst->sst_len));
		goto out;
	}
	(void)memcpy(RCAST(char *, buf) + offs,
	    RCAST(const char *, sst->sst_tab) + pos, len);
	return len;
out:
	errno = EFTYPE;
	return -1;
}

/*
 * Read the sector allocation table.
 */
int
cdf_read_sat(const cdf_info_t *info, cdf_header_t *h, cdf_sat_t *sat)
{
	size_t i, j, k;
	size_t ss = CDF_SEC_SIZE(h);
	cdf_secid_t *msa, mid, sec;
	size_t nsatpersec = (ss / sizeof(mid)) - 1;

	for (i = 0; i < __arraycount(h->h_master_sat); i++)
		if (h->h_master_sat[i] == CDF_SECID_FREE)
			break;

#define CDF_SEC_LIMIT (UINT32_MAX / (64 * ss))
	if ((nsatpersec > 0 &&
	    h->h_num_sectors_in_master_sat > CDF_SEC_LIMIT / nsatpersec) ||
	    i > CDF_SEC_LIMIT) {
		DPRINTF(("Number of sectors in master SAT too big %u %"
		    SIZE_T_FORMAT "u\n", h->h_num_sectors_in_master_sat, i));
		errno = EFTYPE;
		return -1;
	}

	sat->sat_len = h->h_num_sectors_in_master_sat * nsatpersec + i;
	DPRINTF(("sat_len = %" SIZE_T_FORMAT "u ss = %" SIZE_T_FORMAT "u\n",
	    sat->sat_len, ss));
	if ((sat->sat_tab = CAST(cdf_secid_t *, CDF_CALLOC(sat->sat_len, ss)))
	    == NULL)
		return -1;

	for (i = 0; i < __arraycount(h->h_master_sat); i++) {
		if (h->h_master_sat[i] < 0)
			break;
		if (cdf_read_sector(info, sat->sat_tab, ss * i, ss, h,
		    h->h_master_sat[i]) != CAST(ssize_t, ss)) {
			DPRINTF(("Reading sector %d", h->h_master_sat[i]));
			goto out1;
		}
	}

	if ((msa = CAST(cdf_secid_t *, CDF_CALLOC(1, ss))) == NULL)
		goto out1;

	mid = h->h_secid_first_sector_in_master_sat;
	for (j = 0; j < h->h_num_sectors_in_master_sat; j++) {
		if (mid < 0)
			goto out;
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Reading master sector loop limit"));
			goto out3;
		}
		if (cdf_read_sector(info, msa, 0, ss, h, mid) !=
		    CAST(ssize_t, ss)) {
			DPRINTF(("Reading master sector %d", mid));
			goto out2;
		}
		for (k = 0; k < nsatpersec; k++, i++) {
			sec = CDF_TOLE4(CAST(uint32_t, msa[k]));
			if (sec < 0)
				goto out;
			if (i >= sat->sat_len) {
			    DPRINTF(("Out of bounds reading MSA %"
				SIZE_T_FORMAT "u >= %" SIZE_T_FORMAT "u",
				i, sat->sat_len));
			    goto out3;
			}
			if (cdf_read_sector(info, sat->sat_tab, ss * i, ss, h,
			    sec) != CAST(ssize_t, ss)) {
				DPRINTF(("Reading sector %d",
				    CDF_TOLE4(msa[k])));
				goto out2;
			}
		}
		mid = CDF_TOLE4(CAST(uint32_t, msa[nsatpersec]));
	}
out:
	sat->sat_len = i;
	free(msa);
	return 0;
out3:
	errno = EFTYPE;
out2:
	free(msa);
out1:
	free(sat->sat_tab);
	return -1;
}

size_t
cdf_count_chain(const cdf_sat_t *sat, cdf_secid_t sid, size_t size)
{
	size_t i, j;
	cdf_secid_t maxsector = CAST(cdf_secid_t, (sat->sat_len * size)
	    / sizeof(maxsector));

	DPRINTF(("Chain:"));
	if (sid == CDF_SECID_END_OF_CHAIN) {
		/* 0-length chain. */
		DPRINTF((" empty\n"));
		return 0;
	}

	for (j = i = 0; sid >= 0; i++, j++) {
		DPRINTF((" %d", sid));
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Counting chain loop limit"));
			goto out;
		}
		if (sid >= maxsector) {
			DPRINTF(("Sector %d >= %d\n", sid, maxsector));
			goto out;
		}
		sid = CDF_TOLE4(CAST(uint32_t, sat->sat_tab[sid]));
	}
	if (i == 0) {
		DPRINTF((" none, sid: %d\n", sid));
		goto out;

	}
	DPRINTF(("\n"));
	return i;
out:
	errno = EFTYPE;
	return CAST(size_t, -1);
}

int
cdf_read_long_sector_chain(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, cdf_secid_t sid, size_t len, cdf_stream_t *scn)
{
	size_t ss = CDF_SEC_SIZE(h), i, j;
	ssize_t nr;
	scn->sst_tab = NULL;
	scn->sst_len = cdf_count_chain(sat, sid, ss);
	scn->sst_dirlen = MAX(h->h_min_size_standard_stream, len);
	scn->sst_ss = ss;

	if (sid == CDF_SECID_END_OF_CHAIN || len == 0)
		return cdf_zero_stream(scn);

	if (scn->sst_len == CAST(size_t, -1))
		goto out;

	scn->sst_tab = CDF_CALLOC(scn->sst_len, ss);
	if (scn->sst_tab == NULL)
		return cdf_zero_stream(scn);

	for (j = i = 0; sid >= 0; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read long sector chain loop limit"));
			goto out;
		}
		if (i >= scn->sst_len) {
			DPRINTF(("Out of bounds reading long sector chain "
			    "%" SIZE_T_FORMAT "u > %" SIZE_T_FORMAT "u\n", i,
			    scn->sst_len));
			goto out;
		}
		if ((nr = cdf_read_sector(info, scn->sst_tab, i * ss, ss, h,
		    sid)) != CAST(ssize_t, ss)) {
			if (i == scn->sst_len - 1 && nr > 0) {
				/* Last sector might be truncated */
				return 0;
			}
			DPRINTF(("Reading long sector chain %d", sid));
			goto out;
		}
		sid = CDF_TOLE4(CAST(uint32_t, sat->sat_tab[sid]));
	}
	return 0;
out:
	errno = EFTYPE;
	return cdf_zero_stream(scn);
}

int
cdf_read_short_sector_chain(const cdf_header_t *h,
    const cdf_sat_t *ssat, const cdf_stream_t *sst,
    cdf_secid_t sid, size_t len, cdf_stream_t *scn)
{
	size_t ss = CDF_SHORT_SEC_SIZE(h), i, j;
	scn->sst_tab = NULL;
	scn->sst_len = cdf_count_chain(ssat, sid, CDF_SEC_SIZE(h));
	scn->sst_dirlen = len;
	scn->sst_ss = ss;

	if (scn->sst_len == CAST(size_t, -1))
		goto out;

	scn->sst_tab = CDF_CALLOC(scn->sst_len, ss);
	if (scn->sst_tab == NULL)
		return cdf_zero_stream(scn);

	for (j = i = 0; sid >= 0; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read short sector chain loop limit"));
			goto out;
		}
		if (i >= scn->sst_len) {
			DPRINTF(("Out of bounds reading short sector chain "
			    "%" SIZE_T_FORMAT "u > %" SIZE_T_FORMAT "u\n",
			    i, scn->sst_len));
			goto out;
		}
		if (cdf_read_short_sector(sst, scn->sst_tab, i * ss, ss, h,
		    sid) != CAST(ssize_t, ss)) {
			DPRINTF(("Reading short sector chain %d", sid));
			goto out;
		}
		sid = CDF_TOLE4(CAST(uint32_t, ssat->sat_tab[sid]));
	}
	return 0;
out:
	errno = EFTYPE;
	return cdf_zero_stream(scn);
}

int
cdf_read_sector_chain(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, const cdf_sat_t *ssat, const cdf_stream_t *sst,
    cdf_secid_t sid, size_t len, cdf_stream_t *scn)
{

	if (len < h->h_min_size_standard_stream && sst->sst_tab != NULL)
		return cdf_read_short_sector_chain(h, ssat, sst, sid, len,
		    scn);
	else
		return cdf_read_long_sector_chain(info, h, sat, sid, len, scn);
}

int
cdf_read_dir(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, cdf_dir_t *dir)
{
	size_t i, j;
	size_t ss = CDF_SEC_SIZE(h), ns, nd;
	char *buf;
	cdf_secid_t sid = h->h_secid_first_directory;

	ns = cdf_count_chain(sat, sid, ss);
	if (ns == CAST(size_t, -1))
		return -1;

	nd = ss / CDF_DIRECTORY_SIZE;

	dir->dir_len = ns * nd;
	dir->dir_tab = CAST(cdf_directory_t *,
	    CDF_CALLOC(dir->dir_len, sizeof(dir->dir_tab[0])));
	if (dir->dir_tab == NULL)
		return -1;

	if ((buf = CAST(char *, CDF_MALLOC(ss))) == NULL) {
		free(dir->dir_tab);
		return -1;
	}

	for (j = i = 0; i < ns; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read dir loop limit"));
			goto out;
		}
		if (cdf_read_sector(info, buf, 0, ss, h, sid) !=
		    CAST(ssize_t, ss)) {
			DPRINTF(("Reading directory sector %d", sid));
			goto out;
		}
		for (j = 0; j < nd; j++) {
			cdf_unpack_dir(&dir->dir_tab[i * nd + j],
			    &buf[j * CDF_DIRECTORY_SIZE]);
		}
		sid = CDF_TOLE4(CAST(uint32_t, sat->sat_tab[sid]));
	}
	if (NEED_SWAP)
		for (i = 0; i < dir->dir_len; i++)
			cdf_swap_dir(&dir->dir_tab[i]);
	free(buf);
	return 0;
out:
	free(dir->dir_tab);
	free(buf);
	errno = EFTYPE;
	return -1;
}


int
cdf_read_ssat(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, cdf_sat_t *ssat)
{
	size_t i, j;
	size_t ss = CDF_SEC_SIZE(h);
	cdf_secid_t sid = h->h_secid_first_sector_in_short_sat;

	ssat->sat_tab = NULL;
	ssat->sat_len = cdf_count_chain(sat, sid, ss);
	if (ssat->sat_len == CAST(size_t, -1))
		goto out;

	ssat->sat_tab = CAST(cdf_secid_t *, CDF_CALLOC(ssat->sat_len, ss));
	if (ssat->sat_tab == NULL)
		goto out1;

	for (j = i = 0; sid >= 0; i++, j++) {
		if (j >= CDF_LOOP_LIMIT) {
			DPRINTF(("Read short sat sector loop limit"));
			goto out;
		}
		if (i >= ssat->sat_len) {
			DPRINTF(("Out of bounds reading short sector chain "
			    "%" SIZE_T_FORMAT "u > %" SIZE_T_FORMAT "u\n", i,
			    ssat->sat_len));
			goto out;
		}
		if (cdf_read_sector(info, ssat->sat_tab, i * ss, ss, h, sid) !=
		    CAST(ssize_t, ss)) {
			DPRINTF(("Reading short sat sector %d", sid));
			goto out1;
		}
		sid = CDF_TOLE4(CAST(uint32_t, sat->sat_tab[sid]));
	}
	return 0;
out:
	errno = EFTYPE;
out1:
	free(ssat->sat_tab);
	return -1;
}

int
cdf_read_short_stream(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, const cdf_dir_t *dir, cdf_stream_t *scn,
    const cdf_directory_t **root)
{
	size_t i;
	const cdf_directory_t *d;

	*root = NULL;
	for (i = 0; i < dir->dir_len; i++)
		if (dir->dir_tab[i].d_type == CDF_DIR_TYPE_ROOT_STORAGE)
			break;

	/* If the it is not there, just fake it; some docs don't have it */
	if (i == dir->dir_len) {
		DPRINTF(("Cannot find root storage dir\n"));
		goto out;
	}
	d = &dir->dir_tab[i];
	*root = d;

	/* If the it is not there, just fake it; some docs don't have it */
	if (d->d_stream_first_sector < 0) {
		DPRINTF(("No first secror in dir\n"));
		goto out;
	}

	return cdf_read_long_sector_chain(info, h, sat,
	    d->d_stream_first_sector, d->d_size, scn);
out:
	scn->sst_tab = NULL;
	(void)cdf_zero_stream(scn);
	return 0;
}

static int
cdf_namecmp(const char *d, const uint16_t *s, size_t l)
{
	for (; l--; d++, s++)
		if (*d != CDF_TOLE2(*s))
			return CAST(unsigned char, *d) - CDF_TOLE2(*s);
	return 0;
}

int
cdf_read_doc_summary_info(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, const cdf_sat_t *ssat, const cdf_stream_t *sst,
    const cdf_dir_t *dir, cdf_stream_t *scn)
{
	return cdf_read_user_stream(info, h, sat, ssat, sst, dir,
	    "\05DocumentSummaryInformation", scn);
}

int
cdf_read_summary_info(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, const cdf_sat_t *ssat, const cdf_stream_t *sst,
    const cdf_dir_t *dir, cdf_stream_t *scn)
{
	return cdf_read_user_stream(info, h, sat, ssat, sst, dir,
	    "\05SummaryInformation", scn);
}

int
cdf_read_user_stream(const cdf_info_t *info, const cdf_header_t *h,
    const cdf_sat_t *sat, const cdf_sat_t *ssat, const cdf_stream_t *sst,
    const cdf_dir_t *dir, const char *name, cdf_stream_t *scn)
{
	const cdf_directory_t *d;
	int i = cdf_find_stream(dir, name, CDF_DIR_TYPE_USER_STREAM);

	if (i <= 0) {
		memset(scn, 0, sizeof(*scn));
		return -1;
	}

	d = &dir->dir_tab[i - 1];
	return cdf_read_sector_chain(info, h, sat, ssat, sst,
	    d->d_stream_first_sector, d->d_size, scn);
}

int
cdf_find_stream(const cdf_dir_t *dir, const char *name, int type)
{
	size_t i, name_len = strlen(name) + 1;

	for (i = dir->dir_len; i > 0; i--)
		if (dir->dir_tab[i - 1].d_type == type &&
		    cdf_namecmp(name, dir->dir_tab[i - 1].d_name, name_len)
		    == 0)
			break;
	if (i > 0)
		return CAST(int, i);

	DPRINTF(("Cannot find type %d `%s'\n", type, name));
	errno = ESRCH;
	return 0;
}

static const void *
cdf_offset(const void *p, size_t l)
{
	return CAST(const void *, CAST(const uint8_t *, p) + l);
}

static const uint8_t *
cdf_get_property_info_pos(const cdf_stream_t *sst, const cdf_header_t *h,
    const uint8_t *p, const uint8_t *e, size_t i)
{
	size_t tail = (i << 1) + 1;
	size_t ofs;
	const uint8_t *q;

	if (p >= e) {
		DPRINTF(("Past end %p < %p\n", e, p));
		return NULL;
	}
	if (cdf_check_stream_offset(sst, h, p, (tail + 1) * sizeof(uint32_t),
	    __LINE__) == -1)
		return NULL;
	ofs = CDF_GETUINT32(p, tail);
	q = CAST(const uint8_t *, cdf_offset(CAST(const void *, p),
	    ofs - 2 * sizeof(uint32_t)));

	if (q < p) {
		DPRINTF(("Wrapped around %p < %p\n", q, p));
		return NULL;
	}

	if (q >= e) {
		DPRINTF(("Ran off the end %p >= %p\n", q, e));
		return NULL;
	}
	return q;
}

static cdf_property_info_t *
cdf_grow_info(cdf_property_info_t **info, size_t *maxcount, size_t incr)
{
	cdf_property_info_t *inp;
	size_t newcount = *maxcount + incr;

	if (newcount > CDF_PROP_LIMIT) {
		DPRINTF(("exceeded property limit %" SIZE_T_FORMAT "u > %"
		    SIZE_T_FORMAT "u\n", newcount, CDF_PROP_LIMIT));
		goto out;
	}
	inp = CAST(cdf_property_info_t *,
	    CDF_REALLOC(*info, newcount * sizeof(*inp)));
	if (inp == NULL)
		goto out;

	*info = inp;
	*maxcount = newcount;
	return inp;
out:
	free(*info);
	*maxcount = 0;
	*info = NULL;
	return NULL;
}

static int
cdf_copy_info(cdf_property_info_t *inp, const void *p, const void *e,
    size_t len)
{
	if (inp->pi_type & CDF_VECTOR)
		return 0;

	if (CAST(size_t, CAST(const char *, e) - CAST(const char *, p)) < len)
		return 0;

	(void)memcpy(&inp->pi_val, p, len);

	switch (len) {
	case 2:
		inp->pi_u16 = CDF_TOLE2(inp->pi_u16);
		break;
	case 4:
		inp->pi_u32 = CDF_TOLE4(inp->pi_u32);
		break;
	case 8:
		inp->pi_u64 = CDF_TOLE8(inp->pi_u64);
		break;
	default:
		abort();
	}
	return 1;
}

int
cdf_read_property_info(const cdf_stream_t *sst, const cdf_header_t *h,
    uint32_t offs, cdf_property_info_t **info, size_t *count, size_t *maxcount)
{
	const cdf_section_header_t *shp;
	cdf_section_header_t sh;
	const uint8_t *p, *q, *e;
	size_t i, o4, nelements, j, slen, left;
	cdf_property_info_t *inp;

	if (offs > UINT32_MAX / 4) {
		errno = EFTYPE;
		goto out;
	}
	shp = CAST(const cdf_section_header_t *,
	    cdf_offset(sst->sst_tab, offs));
	if (cdf_check_stream_offset(sst, h, shp, sizeof(*shp), __LINE__) == -1)
		goto out;
	sh.sh_len = CDF_TOLE4(shp->sh_len);
	if (sh.sh_len > CDF_SHLEN_LIMIT) {
		errno = EFTYPE;
		goto out;
	}

	if (cdf_check_stream_offset(sst, h, shp, sh.sh_len, __LINE__) == -1)
		goto out;

	sh.sh_properties = CDF_TOLE4(shp->sh_properties);
	DPRINTF(("section len: %u properties %u\n", sh.sh_len,
	    sh.sh_properties));
	if (sh.sh_properties > CDF_PROP_LIMIT)
		goto out;
	inp = cdf_grow_info(info, maxcount, sh.sh_properties);
	if (inp == NULL)
		goto out;
	inp += *count;
	*count += sh.sh_properties;
	p = CAST(const uint8_t *, cdf_offset(sst->sst_tab, offs + sizeof(sh)));
	e = CAST(const uint8_t *, cdf_offset(shp, sh.sh_len));
	if (p >= e || cdf_check_stream_offset(sst, h, e, 0, __LINE__) == -1)
		goto out;

	for (i = 0; i < sh.sh_properties; i++) {
		if ((q = cdf_get_property_info_pos(sst, h, p, e, i)) == NULL)
			goto out;
		inp[i].pi_id = CDF_GETUINT32(p, i << 1);
		left = CAST(size_t, e - q);
		if (left < sizeof(uint32_t)) {
			DPRINTF(("short info (no type)_\n"));
			goto out;
		}
		inp[i].pi_type = CDF_GETUINT32(q, 0);
		DPRINTF(("%" SIZE_T_FORMAT "u) id=%#x type=%#x offs=%#tx,%#x\n",
		    i, inp[i].pi_id, inp[i].pi_type, q - p, offs));
		if (inp[i].pi_type & CDF_VECTOR) {
			if (left < sizeof(uint32_t) * 2) {
				DPRINTF(("missing CDF_VECTOR length\n"));
				goto out;
			}
			nelements = CDF_GETUINT32(q, 1);
			if (nelements > CDF_ELEMENT_LIMIT || nelements == 0) {
				DPRINTF(("CDF_VECTOR with nelements == %"
				    SIZE_T_FORMAT "u\n", nelements));
				goto out;
			}
			slen = 2;
		} else {
			nelements = 1;
			slen = 1;
		}
		o4 = slen * sizeof(uint32_t);
		if (inp[i].pi_type & (CDF_ARRAY|CDF_BYREF|CDF_RESERVED))
			goto unknown;
		switch (inp[i].pi_type & CDF_TYPEMASK) {
		case CDF_NULL:
		case CDF_EMPTY:
			break;
		case CDF_SIGNED16:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int16_t)))
				goto unknown;
			break;
		case CDF_SIGNED32:
		case CDF_BOOL:
		case CDF_UNSIGNED32:
		case CDF_FLOAT:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int32_t)))
				goto unknown;
			break;
		case CDF_SIGNED64:
		case CDF_UNSIGNED64:
		case CDF_DOUBLE:
		case CDF_FILETIME:
			if (!cdf_copy_info(&inp[i], &q[o4], e, sizeof(int64_t)))
				goto unknown;
			break;
		case CDF_LENGTH32_STRING:
		case CDF_LENGTH32_WSTRING:
			if (nelements > 1) {
				size_t nelem = inp - *info;
				inp = cdf_grow_info(info, maxcount, nelements);
				if (inp == NULL)
					goto out;
				inp += nelem;
			}
			for (j = 0; j < nelements && i < sh.sh_properties;
			    j++, i++)
			{
				uint32_t l;

				if (o4 + sizeof(uint32_t) > left)
					goto out;

				l = CDF_GETUINT32(q, slen);
				o4 += sizeof(uint32_t);
				if (o4 + l > left)
					goto out;

				inp[i].pi_str.s_len = l;
				inp[i].pi_str.s_buf = CAST(const char *,
				    CAST(const void *, &q[o4]));

				DPRINTF(("o=%" SIZE_T_FORMAT "u l=%d(%"
				    SIZE_T_FORMAT "u), t=%" SIZE_T_FORMAT
				    "u s=%s\n", o4, l, CDF_ROUND(l, sizeof(l)),
				    left, inp[i].pi_str.s_buf));

				if (l & 1)
					l++;

				slen += l >> 1;
				o4 = slen * sizeof(uint32_t);
			}
			i--;
			break;
		case CDF_CLIPBOARD:
			if (inp[i].pi_type & CDF_VECTOR)
				goto unknown;
			break;
		default:
		unknown:
			memset(&inp[i].pi_val, 0, sizeof(inp[i].pi_val));
			DPRINTF(("Don't know how to deal with %#x\n",
			    inp[i].pi_type));
			break;
		}
	}
	return 0;
out:
	free(*info);
	*info = NULL;
	*count = 0;
	*maxcount = 0;
	errno = EFTYPE;
	return -1;
}

int
cdf_unpack_summary_info(const cdf_stream_t *sst, const cdf_header_t *h,
    cdf_summary_info_header_t *ssi, cdf_property_info_t **info, size_t *count)
{
	size_t maxcount;
	const cdf_summary_info_header_t *si =
	    CAST(const cdf_summary_info_header_t *, sst->sst_tab);
	const cdf_section_declaration_t *sd =
	    CAST(const cdf_section_declaration_t *, RCAST(const void *,
	    RCAST(const char *, sst->sst_tab)
	    + CDF_SECTION_DECLARATION_OFFSET));

	if (cdf_check_stream_offset(sst, h, si, sizeof(*si), __LINE__) == -1 ||
	    cdf_check_stream_offset(sst, h, sd, sizeof(*sd), __LINE__) == -1)
		return -1;
	ssi->si_byte_order = CDF_TOLE2(si->si_byte_order);
	ssi->si_os_version = CDF_TOLE2(si->si_os_version);
	ssi->si_os = CDF_TOLE2(si->si_os);
	ssi->si_class = si->si_class;
	cdf_swap_class(&ssi->si_class);
	ssi->si_count = CDF_TOLE4(si->si_count);
	*count = 0;
	maxcount = 0;
	*info = NULL;
	if (cdf_read_property_info(sst, h, CDF_TOLE4(sd->sd_offset), info,
	    count, &maxcount) == -1)
		return -1;
	return 0;
}


#define extract_catalog_field(t, f, l) \
    if (b + l + sizeof(cep->f) > eb) { \
	    cep->ce_namlen = 0; \
	    break; \
    } \
    memcpy(&cep->f, b + (l), sizeof(cep->f)); \
    ce[i].f = CAST(t, CDF_TOLE(cep->f))

int
cdf_unpack_catalog(const cdf_header_t *h, const cdf_stream_t *sst,
    cdf_catalog_t **cat)
{
	size_t ss = cdf_check_stream(sst, h);
	const char *b = CAST(const char *, sst->sst_tab);
	const char *nb, *eb = b + ss * sst->sst_len;
	size_t nr, i, j, k;
	cdf_catalog_entry_t *ce;
	uint16_t reclen;
	const uint16_t *np;

	for (nr = 0;; nr++) {
		memcpy(&reclen, b, sizeof(reclen));
		reclen = CDF_TOLE2(reclen);
		if (reclen == 0)
			break;
		b += reclen;
		if (b > eb)
		    break;
	}
	if (nr == 0)
		return -1;
	nr--;
	*cat = CAST(cdf_catalog_t *,
	    CDF_MALLOC(sizeof(cdf_catalog_t) + nr * sizeof(*ce)));
	if (*cat == NULL)
		return -1;
	ce = (*cat)->cat_e;
	memset(ce, 0, nr * sizeof(*ce));
	b = CAST(const char *, sst->sst_tab);
	for (j = i = 0; i < nr; b += reclen) {
		cdf_catalog_entry_t *cep = &ce[j];
		uint16_t rlen;

		extract_catalog_field(uint16_t, ce_namlen, 0);
		extract_catalog_field(uint16_t, ce_num, 4);
		extract_catalog_field(uint64_t, ce_timestamp, 8);
		reclen = cep->ce_namlen;

		if (reclen < 14) {
			cep->ce_namlen = 0;
			continue;
		}

		cep->ce_namlen = __arraycount(cep->ce_name) - 1;
		rlen = reclen - 14;
		if (cep->ce_namlen > rlen)
			cep->ce_namlen = rlen;

		np = CAST(const uint16_t *, CAST(const void *, (b + 16)));
		nb = CAST(const char *, CAST(const void *,
		    (np + cep->ce_namlen)));
		if (nb > eb) {
			cep->ce_namlen = 0;
			break;
		}

		for (k = 0; k < cep->ce_namlen; k++)
			cep->ce_name[k] = np[k]; /* XXX: CDF_TOLE2? */
		cep->ce_name[cep->ce_namlen] = 0;
		j = i;
		i++;
	}
	(*cat)->cat_num = j;
	return 0;
}

int
cdf_print_classid(char *buf, size_t buflen, const cdf_classid_t *id)
{
	return snprintf(buf, buflen, "%.8x-%.4x-%.4x-%.2x%.2x-"
	    "%.2x%.2x%.2x%.2x%.2x%.2x", id->cl_dword, id->cl_word[0],
	    id->cl_word[1], id->cl_two[0], id->cl_two[1], id->cl_six[0],
	    id->cl_six[1], id->cl_six[2], id->cl_six[3], id->cl_six[4],
	    id->cl_six[5]);
}



int
cdf_print_property_name(char *buf, size_t bufsiz, uint32_t p)
{
	size_t i;

	for (i = 0; i < __arraycount(vn); i++)
		if (vn[i].v == p)
			return snprintf(buf, bufsiz, "%s", vn[i].n);
	return snprintf(buf, bufsiz, "%#x", p);
}

int
cdf_print_elapsed_time(char *buf, size_t bufsiz, cdf_timestamp_t ts)
{
	int len = 0;
	int days, hours, mins, secs;

	ts /= CDF_TIME_PREC;
	secs = CAST(int, ts % 60);
	ts /= 60;
	mins = CAST(int, ts % 60);
	ts /= 60;
	hours = CAST(int, ts % 24);
	ts /= 24;
	days = CAST(int, ts);

	if (days) {
		len += snprintf(buf + len, bufsiz - len, "%dd+", days);
		if (CAST(size_t, len) >= bufsiz)
			return len;
	}

	if (days || hours) {
		len += snprintf(buf + len, bufsiz - len, "%.2d:", hours);
		if (CAST(size_t, len) >= bufsiz)
			return len;
	}

	len += snprintf(buf + len, bufsiz - len, "%.2d:", mins);
	if (CAST(size_t, len) >= bufsiz)
		return len;

	len += snprintf(buf + len, bufsiz - len, "%.2d", secs);
	return len;
}

char *
cdf_u16tos8(char *buf, size_t len, const uint16_t *p)
{
	size_t i;
	for (i = 0; i < len && p[i]; i++)
		buf[i] = CAST(char, p[i]);
	buf[i] = '\0';
	return buf;
}

