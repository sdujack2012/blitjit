#!/bin/sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=debug -GKDevelop3
cd ..