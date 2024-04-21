#!/usr/bin/env bash
mkdir build;
cd build;
cmake ../ && cmake --build .;
cd ..;
./build/halcyon_test
