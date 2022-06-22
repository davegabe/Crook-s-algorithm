#!/bin/bash
rm -f ./crook.out
gcc crook-pthread.c "../list.c" "../listCount.c" -o crook-pthread.out -lpthread -g -Wall -lm
valgrind --tool=memcheck --leak-check=full "./crook-pthread.out" > log.out 2>&1