#!/bin/sh
cmake -S . -B build/Release \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-G Ninja
