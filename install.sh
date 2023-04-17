#!/bin/bash

# Create build directory and navigate into it
mkdir build && cd build

# Run CMake and build the project
cmake ..
sudo cmake --build . --target install
