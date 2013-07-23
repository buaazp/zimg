#! /bin/bash
gcc -g -levent -levent_openssl -levent_pthreads -lssl -lcrypto -levhtp -lMagickWand-Q16 -lmemcached  zhttpd.c zspinlock.c zlog.c zmd5.c zutil.c zcache.c zimg.c main.c -o main
#for linux
gcc -levhtp -levent -levent_openssl -levent_pthreads -lssl -lcrypto -lMagickWand -lmemcached  zhttpd.c zspinlock.c zlog.c zmd5.c zutil.c zcache.c zimg.c main.c -o main
