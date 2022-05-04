#!/bin/bash

rm -rf *.abi*
rm -rf *.wasm*

eosio-cpp -abigen -I include -R resource -contract lptoken -o lptoken.wasm src/lptoken.cpp

cleos -u https://eospush.mytokenpocket.vip set contract swaplptokent $(pwd)
 
