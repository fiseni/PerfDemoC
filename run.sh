#!/bin/bash

arg1="data/masterParts.txt"
arg2="data/parts.txt"

if [ "$1" = "short" ]; then
  arg1="data/masterPartsShort.txt"
  arg2="data/partsShort.txt"
fi

impl="$2"
if [ "$impl" = "" ]; then
  impl="4"
fi

FLAGS="-O3 -march=native -s -Wall -Wextra -Wno-unused-function"
FILES="main.c utils.c cross_platform_time.c thread_utils.c hash_table_string.c hash_table_sizelist.c source_data.c test.c processor$impl.c"

if [ "$1" = "test" ]; then
  gcc $FLAGS $FILES -o demo && ./demo test $arg2
else
  gcc $FLAGS -DNDEBUG $FILES -o demo && ./demo $arg1 $arg2
fi
