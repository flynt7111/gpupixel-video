#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Remove the build directory if it exists
rm -rf build

# Configure the project with CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build

echo "Build completed successfully."