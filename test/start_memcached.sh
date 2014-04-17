#!/bin/bash
/usr/local/bin/memcached -d -m 1024 -l 127.0.0.1 -p 11211 -c 1024 -P ~/run/memcached.pid
#linux
sudo /usr/sbin/memcached -d -u root -m 1024 -l 127.0.0.1 -p 11211 -c 1024 -P /var/run/memcached.pid
