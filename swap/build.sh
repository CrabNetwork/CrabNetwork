#!/bin/bash

eosio-cpp -abigen -I include -R resource -contract swap -o swap.wasm src/swap.cpp
