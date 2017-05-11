PREFIX=/usr/local/zimg
PWP=$(shell pwd)
ARCH=$(shell getconf LONG_BIT)
SYSTEM=$(shell uname -s)

ifeq ($(ARCH),32)
	cflag32=--host i686-pc-linux-gnu CFLAGS='-O3 -m32' LDFLAGS=-m32
endif

libjpeg-turbo=./deps/libjpeg-turbo/.libs/libjpeg.a
libwebp=./deps/libwebp/src/.libs/libwebp.a
libimagickwand=./deps/ImageMagick/wand/.libs/libMagickWand-6.Q8.a
libluajit=./deps/LuaJIT/src/libluajit.a

ifeq ($(SYSTEM),Darwin)
	deps=$(libluajit)
else
	deps=$(libjpeg-turbo) $(libwebp) $(libimagickwand) $(libluajit)
endif

all: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake $(PWD)/src; make -j 4; cp zimg $(PWD)/bin

debug: $(deps)
	mkdir -p build/zimg
	cd build/zimg; cmake -DCMAKE_BUILD_TYPE=Debug $(PWD)/src; make; cp zimg $(PWD)/bin

$(libjpeg-turbo):
	cd deps; mkdir libjpeg-turbo; tar zxvf libjpeg-turbo-*.tar.gz -C libjpeg-turbo --strip-components 1; cd libjpeg-turbo; autoreconf -fiv; ./configure --enable-shared=no --enable-static=yes $(cflag32); make -j 4

$(libwebp):
	cd deps; mkdir libwebp; tar zxvf libwebp-*.tar.gz -C libwebp --strip-components 1; cd libwebp; ./autogen.sh; ./configure --enable-shared=no --enable-static=yes --with-jpegincludedir=$(PWD)/deps/libjpeg-turbo --with-jpeglibdir=$(PWD)/deps/libjpeg-turbo/.libs; make -j 4

$(libimagickwand):
	cd deps; mkdir ImageMagick; tar zxf ImageMagick-*.tar.gz -C ImageMagick --strip-components 1; cd ImageMagick; ./configure --disable-dependency-tracking --disable-openmp --disable-shared --without-magick-plus-plus --without-fftw --without-fpx --without-djvu --without-fontconfig --without-freetype --without-gslib --without-gvc --without-jbig --without-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-rsvg --without-tiff --without-bzlib --without-wmf --without-xml --without-dps --without-x --enable-delegate-build --with-quantum-depth=8 "CFLAGS=-I$(PWD)/deps/libjpeg-turbo -I$(PWD)/deps/libwebp/src" "LDFLAGS=-L$(PWD)/deps/libjpeg-turbo/.libs -ljpeg -L$(PWD)/deps/libwebp/src/.libs -lwebp"; make -j 4

$(libluajit):
	cd deps; mkdir LuaJIT; tar zxf LuaJIT-*.tar.gz -C LuaJIT --strip-components 1; cd LuaJIT; make -j 4

clean:
	rm -rf build
	rm bin/zimg

cleanall:
	rm -rf build
	rm -f bin/zimg
	rm -rf deps/libjpeg-turbo
	rm -rf deps/libwebp
	rm -rf deps/ImageMagick
	rm -rf deps/LuaJIT
