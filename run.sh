#!/bin/bash

arg1="data/masterParts.txt"
arg2="data/parts.txt"

if [ "$1" = "short" ]; then
  arg1="data/masterPartsShort.txt"
  arg2="data/partsShort.txt"
fi
if [ "$1" = "test" ]; then
  arg1="test"
  arg2=""
fi

impl="$2"
if [ "$impl" = "" ]; then
  impl="4"
fi

FLAGS="-O3 -march=native -s -DNDEBUG -Wall -Wextra -Wno-unused-function"
FILES="main.c utils.c cross_platform_time.c hash_table_string.c hash_table_sizelist.c source_data.c test.c processor$impl.c"

if [ "$type" = "echo" ]; then
  echo gcc $FLAGS $FILES -o demo
else
  gcc $FLAGS $FILES -o demo && ./demo $arg1 $arg2
fi
