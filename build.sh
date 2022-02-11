#!/bin/bash

gcc src/main.c -o bin/wc2 -Wall -g -lraylib -lpthread -ldl -lrt -lX11 -lm -Iinc
