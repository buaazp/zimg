PREFIX=/usr/local/zimg
PWP=$(shell pwd)

all:
	mkdir -p build/libevhtp
	cd build/libevhtp; cmake $(PWD)/dep/libevhtp ; make; cp evhtp-config.h $(PWD)/src/include; cp libevhtp.a $(PWD)/src/lib
	mkdir -p src/include/hiredis
	cd dep/hiredis; make
	cp dep/hiredis/hiredis.h $(PWD)/src/include/hiredis
	cp dep/hiredis/async.h $(PWD)/src/include/hiredis
	cp -r dep/hiredis/adapters $(PWD)/src/include/hiredis
	cp dep/hiredis/libhiredis.a $(PWD)/src/lib
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

clean:
	rm -rf build
	rm src/include/evhtp-config.h
	rm src/lib/libevhtp.a
	cd dep/hiredis; make clean
	rm -rf src/include/hiredis
	rm src/lib/libhiredis.a
	rm bin/zimg
