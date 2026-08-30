#!/bin/sh
# build.sh <folder> -- builds the C CLI app in <folder> using that
# folder's own Makefile. With no argument, builds every folder in
# this repo that has a Makefile.
#
#   ./build.sh research_and_development/tribute_to_tony
#   ./build.sh
set -e

if [ -z "$DBJ_CORELIB" ]; then
    echo "DBJ_CORELIB is not set -- point it at <repo>/corelib"
    exit 1
fi

if [ -n "$1" ]; then
    folder="$1"
    shift
    if [ ! -d "$folder" ]; then
        echo "$folder is not a folder"
        exit 1
    fi
    if [ ! -f "$folder/Makefile" ]; then
        echo "No Makefile in $folder"
        exit 1
    fi
    make -C "$folder" "$@"
    exit $?
fi

# one level down as well: every POC now sits under
# research_and_development/, and the deprecated corner still builds
for dir in */ */*/; do
    dir="${dir%/}"
    if [ -f "$dir/Makefile" ]; then
        echo "=== $dir ==="
        make -C "$dir"
    fi
done
