#!/bin/sh

# patchelf: sudo apt install patchelf -y
# ldd: sudo apt install ldd -y
# patchelf: sudo apt install patchelf
#=====================================

set -e # Exit on error

TARGET=$1

if [ -z "$TARGET" ]; then
    echo "Usage: $0 <target>"
    exit 1
fi

# remove libs if exists
rm -rf libs

# Create a directory for the libs
mkdir -p libs

ldd "$TARGET" > linker_deps.txt

if [ $? -ne 0 ]; then
    echo "Failed to run ldd on the binary"
    exit 1
fi

# Parse ldd output, copy libraries to 'libs' directory, and adjust library paths
awk '/=>/ {print $3} ' linker_deps.txt | grep -v '^\s*$' | while read lib; do
    cp "$lib" ./libs/
    new_lib="./libs/$(basename "$lib")"
    patchelf --set-rpath '$ORIGIN/libs' "$new_lib"
done

rm -f linker_deps.txt