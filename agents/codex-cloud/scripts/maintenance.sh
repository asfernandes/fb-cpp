#!/bin/sh

git submodule update --init --recursive

cp CMakeUserPresets.json.posix.template CMakeUserPresets.json
cmake --preset default
