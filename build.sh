#!/bin/bash

if [ ! -f "./PBCounter" ] 
then
    echo "file not exisit"
else 
    rm PBCounter 
fi

mkdir -p .build && cd .build && cmake .. && make -f Makefile && cp PBCounter ..
