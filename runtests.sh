#!/bin/bash

gcc src/tests/tests.c -o bin/tests -Wall -pg -lraylib -lpthread -ldl -lrt -lX11 -Iinc

if [ $? -eq 0 ];
then
	echo -e "BUILD SUCCEEDED
"
	cd bin
	gdb ./tests -ex=r -q
	run
fi
