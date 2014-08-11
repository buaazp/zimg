PREFIX=/usr/local/zimg
PWP=$(shell pwd)

all:
	mkdir -p src/webp/build
	cd src/webp; ./configure --prefix=$(PWD)/src/webp/build; make; make install
	cd src/LuaJIT; make
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

debug:
	mkdir -p src/webp/build
	cd src/webp; ./configure --prefix=$(PWD)/src/webp/build; make; make install
	cd src/LuaJIT; make
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

clean:
	rm -rf build
	rm bin/zimg
