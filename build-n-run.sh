#!/bin/bash
rm -f ./crook.out
gcc crook-unoptimized.c list.c listCount.c -o crook.out -lm
./crook.out