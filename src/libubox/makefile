
cross_compile ?=
cc ?=$(cross_compile)gcc
ar ?=$(cross_compile)ar
ld ?=$(cross_compile)ld
strip ?=$(cross_compile)strip

target = libubox.so

def                     ?=

cflags                  ?=  -W -Wall -Wchar-subscripts -Wcomment -Wformat -Wimplicit \
                                 -Wmissing-braces -Wparentheses -Wswitch -Wunused -Wunused-parameter  -Wfloat-equal\
                                 -Wsign-compare -Wpointer-arith -Winline -Werror -g
ldflags                 ?=  -rdynamic -fPIC -shared

includes                 ?= -I../../include
libdir                  ?=  -L../../lib
lib                     ?= -lreadline -lhistory  -lpthread -levent_core -levent

obj:=$(patsubst %.c,%.o,$(wildcard *.c))

.PHONY:all
all:$(obj)
	$(cc)    $(ldflags)  $^   $(libdir) $(lib) -o $(target)
%.o:%.c
	$(cc)   $(ldflags) $(includes) $(def) -c -g $< 
.PHONY:install
install:
.PHONY:clean
clean:
	@rm *.o $(target) 2>/dev/null



