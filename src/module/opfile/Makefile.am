ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS=foreign
lib_LTLIBRARIES=libopfile.la

libopfile_la_SOURCES=buffer.c magic.c apprentice.c softmagic.c ascmagic.c \
	encoding.c compress.c is_csv.c is_json.c is_tar.c readelf.c print.c \
	fsmagic.c funcs.c apptype.c der.c fmtcheck.c\
     cdf.c cdf_time.c readcdf.c opfile.c file.c seccomp.c
libopfile_la_includedir=$(prefix)/include/module
libopfile_la_include_HEADERS=opfile.h

libopfile_la_CFLAGS  = -I$(top_srcdir)/src
libopfile_la_CFLAGS += -I$(prefix)/include
libopfile_la_CFLAGS += -g -W -DHAVE_CONFIG_H -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit -Wmissing-braces -Wno-error=unused-result -Wparentheses -Wswitch -Wno-unused-parameter  -Wfloat-equal -Wsign-compare -Wpointer-arith -Winline -Werror  -rdynamic -std=gnu99
libopfile_la_LDFLAGS = -L$(top_builddir)/src/base/.libs
libopfile_la_LDFLAGS += -L$(top_builddir)/src/opbox/.libs
libopfile_la_LDFLAGS = -L$(prefix)/lib
libopfile_la_LIBADD = -lm -lbz2 -lbase -lopbox
