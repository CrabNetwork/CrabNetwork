#/bin/bash

cleos -u https://eospush.tokenpocket.pro set account permission boxgl1111111 \
active '{"threshold":1,"keys":[{"key":"EOS7tufG7LXKqtFFSueMZ3PVyWa46xJNSBDLn7bzv9UtyQDUFnURy","weight":1}],"accounts":[{"permission":{"actor":"boxgl1111111","permission":"eosio.code"},"weight":1}]}' \
-p boxgl1111111@active

cleos -u https://eospush.tokenpocket.pro push action crabfarm1111 \
add '[["0,BOXGL","lptoken.defi"],"boxgl1111111",100, true]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action boxgl1111111 \
init '[["0,BOXGL","lptoken.defi"],["4,EOS","eosio.token"],["6,BOX","token.defi"],["6,BOX","token.defi"],"194","194","crabfarm1111",[100,200,200,10,0],43200]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action crabfarm1111 \
setenabled '[1, true]' -p devpcrabfarm@active


# ===== test ========

cleos -u http://n45i953228.qicp.vip set account permission boxgllp22222 \
active '{"threshold":1,"keys":[{"key":"EOS5zASvvpQj31GZB4drkup2Kd7Z8wA1Gb9Pjwrdkr3kWLxoHW57p","weight":1}],"accounts":[{"permission":{"actor":"boxgllp22222","permission":"eosio.code"},"weight":1}]}' \
-p boxgllp22222@active

cleos -u http://n45i953228.qicp.vip set account permission boxgl1111111 \
active '{"threshold":1,"keys":[{"key":"EOS5zASvvpQj31GZB4drkup2Kd7Z8wA1Gb9Pjwrdkr3kWLxoHW57p","weight":1}],"accounts":[{"permission":{"actor":"boxgl1111111","permission":"eosio.code"},"weight":1}]}' \
-p boxgl1111111@active

cleos -u http://n45i953228.qicp.vip push action crabfarm1111 \
add '[["0,BOXGL","lptoken.defi"],"boxgl1111111",100, true]' -p devpcrabfarm@active

cleos -u http://n45i953228.qicp.vip push action boxgl1111111 \
init '[["0,BOXGL","lptoken.defi"],["4,EOS","eosio.token"],["6,BOX","token.defi"],["6,BOX","token.defi"],"194","194","crabfarm1111",[100,200,200,10,0],43200]' -p devpcrabfarm@active

cleos -u http://n45i953228.qicp.vip push action crabfarm1111 \
setenabled '[1, true]' -p devpcrabfarm@active