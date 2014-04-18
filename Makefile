PREFIX=/usr/local/zimg
PWP=$(shell pwd)

all:
	mkdir -p build/libevhtp
	cd build/libevhtp; cmake $(PWD)/dep/libevhtp ; make; cp evhtp-config.h $(PWD)/src/include; cp libevhtp.a $(PWD)/src/lib
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

clean:
	rm -rf build
	rm src/include/evhtp-config.h
	rm src/lib/libevhtp.a
	rm bin/zimg
