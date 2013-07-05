#! /bin/bash
gcc -levent -lMagickWand-Q16 -lmemcached zconf.c zspinlock.c zlog.c zmd5.c zutil.c zhttpd.c main.c -o main
