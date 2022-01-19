#!/bin/bash

gcc src/main.c -o bin/wc2 -Wall -pg -lraylib -lpthread -ldl -lrt -lX11 -Iinc
