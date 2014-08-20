PREFIX=/usr/local/zimg
PWP=$(shell pwd)

libwebp=./src/libwebp-0.4.0/src/.libs/libwebp.a
libluajit=./src/LuaJIT-2.0.3/src/libluajit.a
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
	cd src; tar zxf LuaJIT-2.0.3.tar.gz; cd LuaJIT-2.0.3; make

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm bin/zimg
	rm -rf src/LuaJIT-2.0.3
	rm -rf src/libwebp-0.4.0
