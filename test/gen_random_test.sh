#!/bin/bash

for ((w=3500;w<=3600;w++)); do
    for ((h=2600;h<=2700;h++)); do
        #echo "GET http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=$w&h=$h&p=0&g=1";
        echo "GET http://127.0.0.1:4869/dedd6ff1e146ac104f190baac091f567?w=$w&h=$h";
    done;
done;
