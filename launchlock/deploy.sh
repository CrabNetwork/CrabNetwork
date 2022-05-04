#!/bin/bash

rm -rf *.abi*
rm -rf *.wasm*

eosio-cpp -abigen -I include -R resource -contract locktime -o locktime.wasm src/locktime.cpp
cleos -u https://eospush.tokenpocket.pro set contract eoslockvants $(pwd)