#!/bin/bash


cleos -u http://eospush.tokenpocket.pro set account permission launchpadddd active \
'{"threshold":1,"keys":[{"key":"EOS6pv39jD3s6NZsQUSGWySwg5yBJQneU4ZgfEfFiR8w1mgLZyek8","weight":1}],"accounts":[{"permission":{"actor":"launchpadddd","permission":"eosio.code"},"weight":1}]}' \
 -p launchpadddd@active

 cleos -u http://eospush.tokenpocket.pro push action eoslockvants remove '[1]' -p eoslockvants
 cleos -u http://eospush.tokenpocket.pro push action eoslockvants withdraw '[1,"bxf1ug5blycg"]' -p bxf1ug5blycg

cleos -u http://eospush.tokenpocket.pro transfer eoslockvants bxf1ug5blycg "2078450969 CBB" -c swaplptokent
cleos -u http://eospush.tokenpocket.pro transfer bxf1ug5blycg eoslockvants "2078450969 CBB" "deposit_launch,1663037243,bxf1ug5blyc" -c swaplptokent




 