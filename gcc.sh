#! /bin/bash
gcc -levent -lMagickWand-Q16 -lmemcached zconf.c zspinlock.c zlog.c zmd5.c zutil.c zcache.c zimg.c zhttpd.c zthread.c zworkqueue.c main.c -o main
#for linux
gcc -levent -lMagickWand -lmemcached zconf.c zspinlock.c zlog.c zmd5.c zutil.c zcache.c zimg.c zhttpd.c zthread.c zworkqueue.c main.c -o main
