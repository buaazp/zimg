#!/bin/bash
sudo /usr/local/bin/memcached -d -m 1024 -u root -l 127.0.0.1 -p 11211 -c 1024 -P /var/run/memcached.pid
