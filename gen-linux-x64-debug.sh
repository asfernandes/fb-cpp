#!/bin/sh
cmake -S . -B build/Debug \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-G Ninja
