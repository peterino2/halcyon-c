#!/usr/bin/env bash
mkdir build_debug;
cd build_debug;
cmake -DCMAKE_BUILD_TYPE="Debug" ../ && cmake --build . && cd .. && ./build_debug/halcyon_test
mkdir TestResults/;
size build_debug/halcyon_test >> TestResults/binarySize

mkdir build_debug;
cd build_debug;
cmake -DCMAKE_BUILD_TYPE="Release" ../ && cmake --build . && cd .. && ./build_debug/halcyon_test
mkdir TestResults/;
size build_debug/halcyon_test >> TestResults/size
