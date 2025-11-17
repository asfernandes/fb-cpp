#!/bin/sh
cmake -S . -B build/Release \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-DVCPKG_TARGET_TRIPLET=x64-linux-custom \
	-DVCPKG_OVERLAY_TRIPLETS=cmake/triplets \
	-G Ninja
