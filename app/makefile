cross_compile ?=
cc ?=$(cross_compile)gcc
ar ?=$(cross_compile)ar
ld ?=$(cross_compile)ld
strip ?=$(cross_compile)strip
install_dir ?=
dirs = opcli oplog webserver
.PHONY:all
all:
	@for i in $(dirs);do make -C $$i all|| exit 1;done

.PHONY:dlib
dlib:
	@for i in $(dirs);do make -C $$i dlib|| exit 1;done
	
.PHONY:install
install:
	@for i in $(dirs);do make -C $$i install|| exit 1;done
.PHONY:install_dlib
install_dlib:
	@for i in $(dirs);do make -C $$i install_dlib|| exit 1;done
.PHONY:clean
clean:
	@for i in $(dirs);do make -C $$i clean|| exit 1;done
.PHONY:distclean
distclean:
	@for i in $(dirs);do make -C $$i distclean|| exit 1;done	
