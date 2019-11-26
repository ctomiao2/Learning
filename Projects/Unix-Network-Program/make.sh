#!/bin/bash
# build client.c
cd ./lib
cmake .
make
cd ../cli_dir
cmake .
make
cd ../srv_dir
cmake .
make
cd ../
g++ -g -w -o client cli_dir/libcli.a lib/libutils.a -std=c++11 -lpthread
g++ -g -w -o server srv_dir/libsrv.a lib/libutils.a -std=c++11 -lpthread

