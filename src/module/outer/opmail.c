#include <stdlib.h>
#include <stdio.h>

#include "opmail.h"
#include "base/oplog.h"
#include "base/opsql.h"
#include "opbox/utils.h"

#include "base/sql_name.h"

struct opmail_info
{
	char smtp_auth_code[256];
};

static struct opmail_info *self = NULL;

void *opmail_init(void)
{
	void *handle = NULL;
	char sql[OPSQL_LEN];
	struct opmail_info *mail = NULL;
	int count = 0;
	char value[256] ;
		
	mail = calloc(1, sizeof(struct opmail_info));
	if (!mail) {
		log_error("calloc failed\n");
		goto out;
	}

	handle = opsql_alloc();
	if (!handle) {
		log_warn("opsql_alloc failed\n");
		goto out;
	}

	snprintf(sql, sizeof(sql),"select %s from %s where map_key='%s';", TAB_KEY_VALUE_ELE_VALUE, TABLE_KEY_VALUE,TAB_VALUE_STMP_KEY);
	log_debug("sql:%s\n",sql);
	count = opsql_query(handle, sql);
	if (count <= 0) {
		log_warn("opsql_query failed\n");
		goto out;
	}

	opsql_bind_col(handle, 1, OPSQL_CHAR, value, sizeof(value));
	memset(value, 0, sizeof(value));
	if(opsql_fetch_scroll(handle, count) < 0) {
		log_warn("opsql_fetch_scroll failed[%d]\n", count);
		goto out;
	}

	log_debug("%s=%s\n", TAB_VALUE_STMP_KEY,value);
	strlcpy(mail->smtp_auth_code, value, sizeof(mail->smtp_auth_code));
	opsql_free(handle);
	self = mail;
	return mail;
out:
	opmail_exit(mail);
	return NULL;
}
void opmail_exit(void *mail)
{
	if (!mail)
		return;

	return;
}

void opmail_send_message(char *to, char *theme, char *content)
{
	if (!to)
		return;

	return;
}
void opmail_send_message_ex(char *to, char *theme, const char *fmt, ...)
{
	return;
}


