PREFIX=/usr/local/zimg
PWP=$(shell pwd)

libwebp=./src/libwebp-0.4.0/src/.libs/libwebp.a
libluajit=src/LuaJIT/src/libluajit.a
deps=$(libwebp) $(libluajit)

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libwebp):
	cd src; tar zxf libwebp-0.4.0.tar.gz; cd libwebp-0.4.0; ./configure --enable-shared=no; make

$(libluajit):
	cd src/LuaJIT; make

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm bin/zimg
	cd src/LuaJIT; make clean
	cd src/libwebp-0.4.0; make clean
