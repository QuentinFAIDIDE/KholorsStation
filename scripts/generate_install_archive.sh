#!/bin/bash

# abort on errors
set -e

cd ..
mkdir -p build
cd build 
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
rm -rf package || true
mkdir package
make DESTDIR=./package/tree install
cp ../res/linux_install.sh package/install.sh
cd package
tar -czvf kholors.tar.gz *
