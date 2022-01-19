#!/bin/bash

gcc src/unittests.c -o bin/unittests -Wall -pg -lraylib -lpthread -ldl -lrt -lX11 -Iinc

if [ 0 -eq 0 ];
then
	echo -e "BUILD SUCCEEDED
"
	cd bin
	gdb ./unittests -ex=r -q
	run
fi
