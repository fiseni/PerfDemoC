#!/bin/bash

type="$1"
impl="$2"

if [ "$type" = "" ]; then
type="test"
fi

if [ "$impl" = "" ]; then
impl="4"
fi

FLAGS="-O3 -march=native -s -Wall -Wextra"
FILES="main.c utils.c cross_platform_time.c hash_table_string.c hash_table_sizelist.c source_data.c test.c processor$impl.c"

if [ "$type" = "echo" ]; then
echo gcc $FLAGS $FILES -o demo
else
gcc $FLAGS $FILES -o demo && ./demo $type
fi
