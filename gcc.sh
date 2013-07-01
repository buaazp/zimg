#! /bin/bash
gcc -g -levent -lMagickWand-Q16 -lcrypto zconf.c zutil.c zhttpd.c main.c -o main
