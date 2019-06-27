#!/bin/bash
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then rm -Rf $BUILD_DIR; fi
mkdir build
cd build
cmake ..
make 
make install
