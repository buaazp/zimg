LIBS = -levhtp -levent -levent_pthreads -lMagickWand -lmemcached
OBJS = zhttpd.o zspinlock.o zlog.o zmd5.o zutil.o zcache.o zimg.o main.o
CFLAGS = -Wall
all: ${OBJS}
	gcc ${CFLAGS} -o zimg ${OBJS} ${LIBS}
clean:
	rm -f zimg ${OBJS}
