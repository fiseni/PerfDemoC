#!/bin/bash

type="$1"
impl="$2"

if [ "$type" = "" ]; then
type="short"
fi

if [ "$impl" = "" ]; then
impl="1"
fi

gcc -O3 -mavx2 -msse2 -o demo main.c common.c cross_platform_time.c test.c processor$impl.c && ./demo $type