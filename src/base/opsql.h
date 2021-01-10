#ifndef __OPSQL_H__
#define __OPSQL_H__

void *opsql_init(char *conf_path);

void opsql_exit(void *_sql);

#endif
