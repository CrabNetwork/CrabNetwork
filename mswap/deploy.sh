#!/bin/bash

rm -rf *.abi*
rm -rf *.wasm*

#cleos -u https://eospush.tokenpocket.pro system buyram bxf1ug5blycg aidaommmswat -k 400
#cleos -u https://eospush.tokenpocket.pro system buyram crabdeployer crabdeployer -k 20
#cleos -u https://eospush.tokenpocket.pro set account permission aidaommmswat active '{"threshold":1,"keys":[{"key":"EOS5A4EjGxnfZysDJXaTykYHcv5SNxgxizpKHuELTx6chHCYjEz1w","weight":1}],"accounts":[{"permission":{"actor":"aidaommmswat","permission":"eosio.code"},"weight":1}]}' -p aidaommmswat@active

eosio-cpp -abigen -I include -R resource -contract mswap -o mswap.wasm src/mswap.cpp
cleos -u https://eospush.tokenpocket.pro set contract aidaommmswat $(pwd)

 
