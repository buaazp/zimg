PREFIX=/usr/local/zimg
PWP=$(shell pwd)

libimagickwand=./deps/ImageMagick-6.9.1-10/wand/.libs/libMagickWand-6.Q8.a
libluajit=./deps/LuaJIT-2.0.3/src/libluajit.a
deps=$(libimagickwand) $(libluajit)

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libimagickwand):
	cd deps; tar zxf ImageMagick.tar.gz; cd ImageMagick-6.9.1-10; ./configure --disable-dependency-tracking --disable-openmp --disable-shared --without-magick-plus-plus --without-fftw --without-fpx --without-djvu --without-fontconfig --without-freetype --without-gslib --without-gvc --without-jbig --without-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-rsvg --without-tiff --without-bzlib --without-wmf --without-xml --without-dps --without-x --enable-delegate-build --with-quantum-depth=8; make -j 4

$(libluajit):
	cd deps; tar zxf LuaJIT-2.0.3.tar.gz; cd LuaJIT-2.0.3; make

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm -f bin/zimg
	rm -rf deps/ImageMagick-6.9.1-10
	rm -rf deps/LuaJIT-2.0.3
