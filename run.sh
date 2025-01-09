#!/bin/bash

impl="$1"
type="$2"

if [ "$type" = "" ]; then
type="test"
fi

if [ "$impl" = "" ]; then
impl="1"
fi

gcc -O3 -mavx2 -msse2 -o demo main.c common.c cross_platform_time.c test.c processor$impl.c && ./demo $type