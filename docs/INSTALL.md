# Installation Guide

## Prerequisites
- CMake (>= 3.16)
- ROOT (>= 6.24)
- nlohmann JSON

## Build Instructions
```sh
mkdir build && cd build
cmake ..
make -j$(nproc)
