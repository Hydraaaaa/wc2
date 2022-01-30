#!/bin/bash

gcc src/runtests.c -o bin/runtests -Wall -pg -lraylib -lpthread -ldl -lrt -lX11 -Iinc

if [ $? -eq 0 ];
then
	echo -e "BUILD SUCCEEDED
"
	cd bin
	gdb ./runtests -ex=r -q
	run
fi
