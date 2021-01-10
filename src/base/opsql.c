#include<stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "opsql.h"
#include "odbc/sql.h"
#include "odbc/sqlext.h"
#include "odbc/sqltypes.h"
#include "iniparser.h"
#include "opbox/utils.h"

#define OPSQL_SERVER "opsql:path"
#define SQL_ODBCSYSINI "ODBCSYSINI"
#define SQL_ODBCINI "ODBCINI"

struct _opsql_struct {
	HENV henv;
	SQLHDBC hdbc;
	SQLHSTMT stmt;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	char path[128];
};

void *opsql_init(char *conf_path)
{

	struct _opsql_struct *_sql = NULL;
	SQLRETURN rcode = 0;
	dictionary *dict = NULL;
	const char * str = NULL;
	char buf_tmp[240] = {};

	printf ("%s %d opsql_init init[conf_path=%s]\n",__FILE__,__LINE__, conf_path);

	_sql = calloc(1, sizeof(struct _opsql_struct));
	if (!_sql) {
		printf ("%s %d calloc failed[%d]\n",__FILE__,__LINE__, errno);
		goto exit;
	}
	
	
	dict = iniparser_load(conf_path);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FILE__,__LINE__, conf_path);
		goto exit;
	}

	if(!(str = iniparser_getstring(dict,OPSQL_SERVER,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, OPSQL_SERVER);
		iniparser_freedict(dict);
		goto exit;
	}

	printf ("%s %d odbc etc =%s\n",__FILE__,__LINE__, str);

	strlcpy(_sql->path, str, sizeof(_sql->path));

	iniparser_freedict(dict);

	snprintf(buf_tmp, sizeof(buf_tmp),"%s/odbc.ini", _sql->path);
	setenv(SQL_ODBCSYSINI, _sql->path, 1);
	setenv(SQL_ODBCINI, buf_tmp, 1);



	rcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_sql->henv);
	if (rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO) {
		printf ("%s %d SQLAllocHandle failed\n",__FILE__,__LINE__);
		goto exit;
	}

	rcode = SQLSetEnvAttr(_sql->henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if (rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO) {
		printf ("%s %d SQLSetEnvAttr failed\n",__FILE__,__LINE__);
		goto exit;
	}

	rcode = SQLAllocHandle(SQL_HANDLE_DBC, _sql->henv, &_sql->hdbc);
	if (rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO) {
		printf ("%s %d SQLAllocHandle failed\n",__FILE__,__LINE__);
		goto exit;
	}

	rcode = SQLConnect(_sql->hdbc, (SQLCHAR *)"isir", SQL_NTS,
		(SQLCHAR *)"isir", SQL_NTS, (SQLCHAR *)"isir", SQL_NTS);

	if (rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO) {
		printf ("%s %d SQLConnect failed\n",__FILE__,__LINE__);
		goto exit;
	}

	rcode = SQLAllocHandle(SQL_HANDLE_STMT, _sql->hdbc,&_sql->stmt);
	if (rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO) {
		printf ("%s %d SQLAllocHandle failed\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutexattr_init(&_sql->attr)) {
		printf ("%s %d pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&_sql->lock, &_sql->attr)) {
		printf ("%s %d pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

#if 0
	// *) 执行具体的sql
	rcode = SQLExecDirect(stmt, (SQLCHAR*)"select * from tb_student", SQL_NTS);
	assert(!(rcode != SQL_SUCCESS && rcode != SQL_SUCCESS_WITH_INFO));
 
	// *) 绑定和获取具体的数据项
	SQLINTEGER res = SQL_NTS;
	SQLCHAR name[128];
	SQLINTEGER age;
	SQLBindCol(stmt, 2, SQL_C_CHAR, name, sizeof(name), &res);
	SQLBindCol(stmt, 3, SQL_C_SLONG, &age, sizeof(age), &res);
 
	while ((rcode=SQLFetch(stmt))!=SQL_NO_DATA_FOUND) {
		if( rcode == SQL_ERROR) {
			printf("sql error!\n");
		} else {
			printf("name:%s, age:%ld\n",name, age);
		}
	}
 
#endif
	return _sql;
exit:
	opsql_exit(_sql);

	return NULL;
 
} 


void opsql_exit(void *_sql)
{
	//清理工作, 释放具体的资源句柄
	//SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	//SQLDisconnect(hdbc);
	//SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	//SQLFreeHandle(SQL_HANDLE_ENV, henv);
	return;
}


