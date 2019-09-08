
cross_compile ?=
cc ?=$(cross_compile)gcc
ar ?=$(cross_compile)ar
ld ?=$(cross_compile)ld
strip ?=$(cross_compile)strip
install_dir ?=
target = op_webserver

def                     ?=

cflags                  ?=  -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit \
                                 -Wmissing-braces -Wparentheses -Wswitch -Wunused -Wunused-parameter  -Wfloat-equal\
                                 -Wsign-compare -Wpointer-arith -Winline -Werror -g
ldflags                 ?=  -rdynamic -pthread

includes                 ?=-I../../include
libdir                  ?=  -L../../lib
lib                     ?= -loplog -lubox -levent_core -lzlog -liniparser -levent -levent_pthreads  -lxml
srcs					?= 

obj:=$(patsubst %.c,%.o,$(wildcard *.c))

.PHONY:all
all:$(obj)
	$(cc)    $(ldflags)  $^   $(libdir) $(lib) -o $(target)
%.o:%.c
	$(cc)  $(cflags) $(ldflags) $(includes) $(def) -c -g $< 

.PHONY:dlib
.PHONY:install_dlib

.PHONY:install
install:
	@cp $(target) $(install_dir)/bin
	@cp webserver.xml $(install_dir)/etc

.PHONY:clean
clean:
	@rm *.o $(target) 2>/dev/null



