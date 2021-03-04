#!/bin/sh

# Assume we are in the "soduco-server" directory
echo "Creating build dir"
mkdir -p build && cd build
echo "conan: install deps"
conan install -u .. --build missing -s compiler.libcxx=libstdc++11 -s compiler.cppstd=20
echo "cmake: generate build scripts"
cmake .. -G Ninja -DPYTHON_EXECUTABLE=$(which python3) -DCMAKE_BUILD_TYPE=Release
echo "cmake: launch build"
cmake --build . --parallel --config Release
echo "cpack: create artefacts"
cpack -G ZIP -G TGZ .    
echo "**********************"
echo "BUILD COMPLETE"
echo "**********************"
