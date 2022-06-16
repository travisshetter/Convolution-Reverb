#!/bin/bash
gcc -o conv_reverb conv_reverb.c convolve.c \
	-I/usr/local/include \
	-L/usr/local/lib -lsndfile

