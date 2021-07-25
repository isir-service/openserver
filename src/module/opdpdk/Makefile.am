ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS=foreign
lib_LTLIBRARIES=libopdpdk.la

libopdpdk_la_SOURCES=opdpdk.c
libopdpdk_la_includedir=$(prefix)/include/module
libopdpdk_la_include_HEADERS=opdpdk.h

libopdpdk_la_CFLAGS  = -I$(top_srcdir)/src
libopdpdk_la_CFLAGS += -I$(prefix)/include
libopdpdk_la_CFLAGS += -I$(prefix)/include/libpcap
libopdpdk_la_CFLAGS += -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit -Wmissing-braces -Wno-error=unused-result -Wparentheses -Wswitch -Wno-unused-parameter  -Wfloat-equal -Wsign-compare -Wpointer-arith -Winline -Werror  -rdynamic -std=gnu99

libopdpdk_la_LDFLAGS = -L$(top_builddir)/src/opbox/.libs
libopdpdk_la_LDFLAGS += -L$(top_builddir)/src/base/.libs
libopdpdk_la_LDFLAGS += -L$(prefix)/lib
libopdpdk_la_LIBADD = -lpthread -levent -lbase -lpcap