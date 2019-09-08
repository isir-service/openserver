
cross_compile ?=
cc ?=$(cross_compile)gcc
ar ?=$(cross_compile)ar
ld ?=$(cross_compile)ld
strip ?=$(cross_compile)strip
install_dir  ?=
target = opcli

def                     ?=

cflags                  ?=  -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit \
                                 -Wmissing-braces -Wparentheses -Wswitch -Wunused -Wunused-parameter  -Wfloat-equal\
                                 -Wsign-compare -Wpointer-arith -Winline -Werror -g
ldflags                 ?=  -rdynamic -pthread

includes                 ?=-I../../include
libdir                  ?=  -L../../lib
lib                     ?= -lcli

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
install:
.PHONY:clean
clean:
	@rm *.o $(target) 2>/dev/null



