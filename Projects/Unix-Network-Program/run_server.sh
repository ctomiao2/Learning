#!/bin/bash
# build server.c
gcc -o server server.c -w -lunp
./server $1


