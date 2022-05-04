#!/bin/bash

eosio-cpp -abigen -I include -R resource -contract farm -o farm.wasm src/farm.cpp
cleos -u https://eospush.mytokenpocket.vip set contract swapswapfarm $(pwd)
