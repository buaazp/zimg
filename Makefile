PREFIX=/usr/local/zimg
PWP=$(shell pwd)

libluajit=./deps/LuaJIT-2.0.3/src/libluajit.a
deps=$(libluajit)

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libluajit):
	cd deps; tar zxf LuaJIT-2.0.3.tar.gz; cd LuaJIT-2.0.3; make

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm -f bin/zimg
	rm -rf deps/LuaJIT-2.0.3
