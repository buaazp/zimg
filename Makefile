PREFIX=/usr/local/zimg
PWP=$(shell pwd)

libwebp=./src/webp/src/.libs/libwebp.a
libluajit=src/LuaJIT/src/libluajit.a
deps=$(libwebp) $(libluajit)

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libwebp):
	cd src/webp; ./configure --enable-shared=no; make

$(libluajit):
	cd src/LuaJIT; make

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	cd src/webp; make clean
	cd src/LuaJIT; make clean
	rm -rf build
	rm bin/zimg
