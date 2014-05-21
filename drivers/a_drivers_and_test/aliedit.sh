#!/bin/bash

for dir in $(ls)
do
    [ -d $dir ] && make clean -C $dir
done
