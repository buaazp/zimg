#!/bin/zsh
ab -c 1 -n 1 -v 3 -H "Connection: close" -T "multipart/form-data; boundary=---1234abcd"  -p testup_ab.txt http://127.0.0.1:4869/upload
