#!/bin/bash
rm -f ./crook-pthread.out
gcc crook-pthread.c -o crook-pthread.out -lpthread -g -Wall -lm
./crook-pthread.out