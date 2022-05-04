#!/bin/bash

eosio-cpp -abigen -I include -R resource -contract mswap -o mswap.wasm src/mswap.cpp
