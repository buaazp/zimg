#!/bin/bash

for i in `seq 1000`
do
    echo "====================start" >> press.up
    curl -F "blob=@testup.jpeg;type=image/jpeg" "http://127.0.0.1:8080/upload" >> press.up
    echo "=====================done" >> press.up
done

