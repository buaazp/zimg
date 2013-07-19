#!/bin/bash

for i in `seq 10000`
do
    echo "====================start" >> press.down
    curl http://127.0.0.1:4869/upload.html >> press.down
    echo "=====================done" >> press.down
done

