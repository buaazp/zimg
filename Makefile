PREFIX=/usr/local/zimg
PWP=$(shell pwd)

all:
	mkdir -p build/zimg
	#cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

clean:
	rm -rf build
	rm bin/zimg
