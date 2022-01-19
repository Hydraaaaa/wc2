#!/bin/bash

gcc src/main.c -o bin/wc2 -Wall -pg -lraylib -lpthread -ldl -lrt -lX11 -Iinc

if [ 0 -eq 0 ];
then
	echo -e "BUILD SUCCEEDED
"
	cd bin
	gdb ./wc2 -ex=r
	run
fi
