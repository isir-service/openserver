
cross_compile ?=
cc ?=$(cross_compile)gcc
ar ?=$(cross_compile)ar
ld ?=$(cross_compile)ld
strip ?=$(cross_compile)strip
install_dir ?=
target = oplog
target_lib = liboplog.so

def                     ?=

cflags                  ?=  -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit \
                                 -Wmissing-braces -Wparentheses -Wswitch -Wunused -Wunused-parameter  -Wfloat-equal\
                                 -Wsign-compare -Wpointer-arith -Winline -Werror -g
ldflags                 ?=  -rdynamic -pthread

includes                 ?=-I../../include
libdir                  ?=  -L../../lib
lib                     ?= -lubox -levent_core -lzlog -liniparser -levent -levent_pthreads 
srcs					= oplog.c log_job.c

obj:=$(patsubst %.c,%.o,$(wildcard *.c))

.PHONY:all
all:$(obj)
	$(cc)    $(ldflags)  $^   $(libdir) $(lib) -o $(target)
%.o:%.c
	$(cc)  $(cflags) $(ldflags) $(includes) $(def) -c -g $< 
.PHONY:install
install:
	@cp oplog.conf zlog.conf $(install_dir)/etc
	@cp $(target) $(install_dir)/bin

.PHONY:dlib
dlib:
	$(cc) $(cflags) $(ldflags) $(includes) $(libdir) $(lib) -fPIC -shared $(srcs) -o $(target_lib) 
.PHONY:clean

.PHONY:install_dlib
install_dlib:
	@cp oplog.conf zlog.conf $(install_dir)/etc
	@cp $(target_lib) $(install_dir)/../lib
	
.PHONY:clean	
clean:
	@rm *.o $(target) 2>/dev/null



