#!/bin/bash

mkdir build
clang src/* -o build/main
./build/main
rm -rf build
