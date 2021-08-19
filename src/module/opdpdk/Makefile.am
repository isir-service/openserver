AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=opdpdk
opdpdk_SOURCES=opdpdk.c

opdpdk_LDFLAGS = -L$(prefix)/lib 
opdpdk_LDFLAGS +=-L$(top_builddir)/src/base/.libs
opdpdk_LDFLAGS +=-L$(top_builddir)/src/opbox/.libs
opdpdk_includedir=$(prefix)/include/module/opdpdk
opdpdk_include_HEADERS = 

opdpdk_CFLAGS  = -I$(top_srcdir)/src/
opdpdk_CFLAGS += -I$(prefix)/include
opdpdk_CFLAGS += -I$(prefix)/include/libpcap
opdpdk_CFLAGS += -g -DWITH_BROKER -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit -Wmissing-braces \
	-Wno-error=unused-result -Wparentheses -Wswitch -Wno-unused-parameter  -Wfloat-equal -Wsign-compare -Wpointer-arith -Winline -Werror -rdynamic -std=gnu99

opdpdk_LDADD= -liniparser -lpcap -lpthread -lbase -lopbox -lpthread  -ldl

