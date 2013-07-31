OS = $(shell uname)
MAC = Darwin

ifeq ($(OS),Darwin)
	LIBS = -levent -levent_openssl -levent_pthreads -lssl -lcrypto -levhtp -lMagickWand-6.Q16 -lmemcached
else
	LIBS = -levent -levent_openssl -levent_pthreads -lssl -lcrypto -levhtp -lMagickWand -lmemcached
endif
OBJS = zhttpd.o zspinlock.o zlog.o zmd5.o zutil.o zcache.o zimg.o main.o
CFLAGS = -Wall
all: ${OBJS}
	gcc ${CFLAGS} -o zimg ${OBJS} ${LIBS}
clean:
	rm -f zimg ${OBJS}
