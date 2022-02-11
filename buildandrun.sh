#!/bin/bash

gcc src/main.c -o bin/wc2 -Wall -g -lraylib -lpthread -ldl -lrt -lX11 -lm -Iinc

if [ $? -eq 0 ];
then
	echo -e "BUILD SUCCEEDED\n"
	cd bin
	gdb ./wc2 -ex=r -q
	run
fi
