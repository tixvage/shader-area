#!/bin/sh

set -xe

CFLAGS+="-O0 -ggdb "
CFLAGS+="-lraylib -lm "
CFLAGS+="-Wall -Wextra "

gcc main.c -o main.out $CFLAGS
