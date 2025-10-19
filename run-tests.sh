#!/usr/bin/env bash

set -a
env_file="$(dirname "$0")/.env"
if [ -f "$env_file" ]; then
  source "$env_file"
fi
set +a

mkdir -p $(dirname "$0")/temp/lock
export FIREBIRD_LOCK=$(dirname "$0")/temp/lock

./build/Debug/out/bin/fb-cpp-test "$@"
