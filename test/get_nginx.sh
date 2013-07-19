#!/bin/bash

for i in `seq 10000`
do
    echo "====================start" >> press.down
    curl http://127.0.0.1:8080/5f189d8ec57f5a5a0d3dcba47fa797e2.jpg >> press.down
    echo "=====================done" >> press.down
done

