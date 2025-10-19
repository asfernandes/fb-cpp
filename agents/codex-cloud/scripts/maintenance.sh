#!/bin/sh

git submodule update --init --recursive

./gen-debug.sh
