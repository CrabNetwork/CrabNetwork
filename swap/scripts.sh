#!/bin/bash

 cleos -u https://eospush.mytokenpocket.vip set account permission eosaidaoswap active \
 '{"threshold":1,"keys":[{"key":"EOS5A4EjGxnfZysDJXaTykYHcv5SNxgxizpKHuELTx6chHCYjEz1w","weight":1}],"accounts":[{"permission":{"actor":"eosaidaoswap","permission":"eosio.code"},"weight":1}]}' -p eosaidaoswap@active

cleos -u https://eospush.mytokenpocket.vip push action eosaidaoswat setnotifiers '[["crabtrademia"]]' -p wdogdeployer@active
cleos -u https://eospush.mytokenpocket.vip push action eosaidaoswat setpairnotif '[0, ["swapswapfarm"]]' -p wdogdeployer@active

#danchortoken,eosiotptoken,pizzatotoken
cleos -u https://eospush.mytokenpocket.vip push action eosaidaoswat rmpair '[2]' -p eosaidaoswat
cleos -u https://eospush.mytokenpocket.vip push action eosaidaoswat alert '[2]' -p eosaidaoswat

cleos -u https://eospush.mytokenpocket.vip transfer bxf1ug5blycg eosaidaoswat "1.0000 EOS" "swap,0,1" -p bxf1ug5blycg
cleos -u https://eospush.mytokenpocket.vip transfer bxf1ug5blycg eosaidaoswat "1.0000 USDT" "swap,0,5" -c tethertether -p bxf1ug5blycg

cleos -u https://eospush.mytokenpocket.vip push action swap.defi newmarket '["hello.defi", ["4,EOS","eosio.token"],["4,USDT","tethertether"]]' -p hello.defi
cleos -u https://eospush.mytokenpocket.vip transfer bxf1ug5blycg l55rq3cebsrv "100 EOS" 









