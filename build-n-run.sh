#!/bin/bash
rm -f ./crook.out
gcc crook-unoptimized.c -o crook.out -lm
./crook.out