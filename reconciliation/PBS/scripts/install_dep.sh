#!/usr/bin/env bash

echo "Downloading minisketch ..."
cd /tmp/
git clone https://github.com/sipa/minisketch.git
cd minisketch && ./autogen.sh && ./configure
echo "Start to compile ..."
make && sudo make install

echo "Adding minisketch to LD_LIBRARY_PATH"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib


echo "Downloading xxHash ..."
cd /tmp/
git clone https://github.com/Cyan4973/xxHash.git
cd xxHash 
make && sudo make install