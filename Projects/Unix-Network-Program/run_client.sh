#!/bin/bash
# build client.c
gcc -o client client.c -lunp
./client $1


