#!/bin/bash

mkdir build
clang src/* -o build/main 2> build.log
./build/main
