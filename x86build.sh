#!/bin/bash
mkdir build
cd build
#cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=Debug ..
make Meow -j4 
