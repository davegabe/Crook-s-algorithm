#!/bin/bash
rm -f ./crook.out
gcc crook-unoptimized.c list.c listCount.c -o crook.out -lm
valgrind --tool=memcheck --leak-check=full "./crook.out" > log.out 2>&1