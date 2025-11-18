#!/bin/sh

failed_files=$( \
	find src -type f \( -name "*.cpp" -o -name "*.h" \) \
		-exec sh -c 'for f; do [ "$(clang-format "$f")" != "$(cat "$f")" ] && echo "$f"; done' _ {} +)

if [ -n "$failed_files" ]; then
	echo "The following files are not properly formatted:"
	echo "$failed_files"
	exit 1
fi
