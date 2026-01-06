#!/usr/bin/env bash
set -e

cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
./order_book
