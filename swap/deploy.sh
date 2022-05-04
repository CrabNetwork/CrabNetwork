#!/bin/bash

rm -rf *.abi*
rm -rf *.wasm*

eosio-cpp -abigen -I include -R resource -contract swap -o swap.wasm src/swap.cpp
cleos -u https://eospush.mytokenpocket.vip set contract eosaidaoswat $(pwd)

 
