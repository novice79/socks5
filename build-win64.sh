#!/bin/sh
# set -x
set -e

dir="_build_win64"
[[ "$#" -gt 0 ]] && rm -rf $dir

# -DCMAKE_TOOLCHAIN_FILE="$HOME/clib-prebuilt/cmake-build/win64.cmake" \
# x86_64-w64-mingw32-gcc --print-sysroot
# SET (CMAKE_C_COMPILER_WORKS 1)
# SET (CMAKE_CXX_COMPILER_WORKS 1)
# set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)
# set(CMAKE_C_LINK_FLAGS "")

# brew install mingw-w64 ninja

cmake -GNinja -H. -B$dir \
-DCMAKE_SYSTEM_NAME=Windows \
-DCMAKE_FIND_ROOT_PATH="$HOME/clib-prebuilt/win64" \
-DCMAKE_TRY_COMPILE_TARGET_TYPE="STATIC_LIBRARY" \
-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
-DCMAKE_CXX_LINK_FLAGS="" \
-DCMAKE_INSTALL_PREFIX=dist \
-DCMAKE_BUILD_TYPE=Release 

cmake --build $dir
cmake --install $dir --strip
cmake --build $dir --target package