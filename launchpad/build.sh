#!/bin/bash

rm -rf *.abi*
rm -rf *.wasm*

eosio-cpp -abigen -I include -R resource -contract launchpad -o launchpad.wasm src/launchpad.cpp