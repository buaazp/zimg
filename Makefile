PREFIX=/usr/local/zimg
PWP=$(shell pwd)
ARCH=$(shell getconf LONG_BIT)
SYSTEM=$(shell uname -s)

ifeq ($(ARCH),32)
	cflag32=--host i686-pc-linux-gnu CFLAGS='-O3 -m32' LDFLAGS=-m32
endif

libjpeg-turbo=./deps/libjpeg-turbo-1.4.2/.libs/libjpeg.a
libwebp=./deps/libwebp-0.4.3/src/.libs/libwebp.a
libimagickwand=./deps/ImageMagick-6.9.1-10/wand/.libs/libMagickWand-6.Q8.a
libluajit=./deps/LuaJIT-2.0.3/src/libluajit.a

ifeq ($(SYSTEM),Darwin)
	deps=$(libimagickwand) $(libluajit)
else
	EXTRA_FLAGS="CFLAGS=-I$(PWD)/deps/libjpeg-turbo-1.4.2 -I$(PWD)/deps/libwebp-0.4.3/src" "LDFLAGS=-L$(PWD)/deps/libjpeg-turbo-1.4.2/.libs -ljpeg -L$(PWD)/deps/libwebp-0.4.3/src/.libs -lwebp"
	deps=$(libjpeg-turbo) $(libwebp) $(libimagickwand) $(libluajit)
endif

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make -j 4; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libjpeg-turbo):
	cd deps; tar zxvf libjpeg-turbo-1.4.2.tar.gz; cd libjpeg-turbo-1.4.2; autoreconf -fiv; ./configure --enable-shared=no --enable-static=yes $(cflag32); make -j 4

$(libwebp):
	cd deps; tar zxvf libwebp-0.4.3.tar.gz; cd libwebp-0.4.3; ./configure --enable-shared=no --enable-static=yes --with-jpegincludedir=$(PWD)/deps/libjpeg-turbo-1.4.2 --with-jpeglibdir=$(PWD)/deps/libjpeg-turbo-1.4.2/.libs; make -j 4

$(libimagickwand):
	cd deps; tar zxf ImageMagick.tar.gz; cd ImageMagick-6.9.1-10; ./configure --disable-dependency-tracking --disable-openmp --disable-shared --without-magick-plus-plus --without-fftw --without-fpx --without-djvu --without-fontconfig --without-freetype --without-gslib --without-gvc --without-jbig --without-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-rsvg --without-tiff --without-bzlib --without-wmf --without-xml --without-dps --without-x --enable-delegate-build --with-quantum-depth=8 $(EXTRA_FLAGS); make -j 4

$(libluajit):
	cd deps; tar zxf LuaJIT-2.0.3.tar.gz; cd LuaJIT-2.0.3; make -j 4

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm -f bin/zimg
	rm -rf deps/libjpeg-turbo-1.4.2
	rm -rf deps/libwebp-0.4.3
	rm -rf deps/ImageMagick-6.9.1-10
	rm -rf deps/LuaJIT-2.0.3
