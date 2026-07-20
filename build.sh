#!/bin/sh
# build.sh <folder> -- builds the C CLI app in <folder> using that
# folder's own Makefile. With no argument, builds every folder in
# this repo that has a Makefile.
#
#   ./build.sh tribute_to_tony
#   ./build.sh
set -e

if [ -n "$1" ]; then
    folder="$1"
    shift
    if [ ! -f "$folder/Makefile" ]; then
        echo "No Makefile in $folder"
        exit 1
    fi
    make -C "$folder" "$@"
    exit $?
fi

for dir in */; do
    dir="${dir%/}"
    if [ -f "$dir/Makefile" ]; then
        echo "=== $dir ==="
        make -C "$dir"
    fi
done
