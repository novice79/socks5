#!/bin/sh
# set -x
set -e

dir="_build"
[[ "$#" -gt 0 ]] && rm -rf $dir

cmake -GNinja -H. -B$dir \
-DCMAKE_FIND_ROOT_PATH="$HOME/clib-prebuilt/macos" \
-DCMAKE_INSTALL_PREFIX=dist \
-DCMAKE_BUILD_TYPE=Release 

cmake --build $dir
cmake --install $dir --strip
cmake --build $dir --target package