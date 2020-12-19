ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS=foreign
lib_LTLIBRARIES=libbase.la
libbase_la_SOURCES=oplog.c
libbase_la_includedir=$(prefix)/include/base
libbase_la_include_HEADERS=oplog.h

#libdir=$(prefix)/lib/opbox
#lib=libopbox.la
libbase_la_CFLAGS = -I$(prefix)/include
libbase_la_CFLAGS += -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit -Wmissing-braces -Wparentheses -Wswitch -Wno-unused-parameter  -Wfloat-equal -Wsign-compare -Wpointer-arith -Winline -Werror  -rdynamic

libbase_la_LDFLAGS = -L$(top_builddir)/src/opbox/.libs
libbase_la_LDFLAGS += -L$(prefix)/lib
libbase_la_LIBADD = -liniparser -lpthread -lopbox -levent