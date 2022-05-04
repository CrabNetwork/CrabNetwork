#/bin/bash

cleos -u https://eospush.tokenpocket.pro set account permission boxgvlp22222 \
active '{"threshold":1,"keys":[{"key":"EOS7tufG7LXKqtFFSueMZ3PVyWa46xJNSBDLn7bzv9UtyQDUFnURy","weight":1}],"accounts":[{"permission":{"actor":"boxlendboxgv","permission":"eosio.code"},"weight":1}]}' \
-p boxgvlp22222@active

cleos -u https://eospush.tokenpocket.pro push action crabfarm1111 \
add '[["0,BOXGV","lptoken.defi"],"boxgvlp22222",100, true]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action boxgvlp22222 \
init '[["0,BOXGV","lptoken.defi"],["4,USN","danchortoken"],["6,BOX","token.defi"],["6,BOX","token.defi"],"204","204","crabfarm1111",[100,200,200,10,0],43200]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action crabfarm1111 \
setenabled '[1, true]' -p devpcrabfarm@active


#======= test ============

cleos -u http://n45i953228.qicp.vip set account permission boxlendboxgv \
active '{"threshold":1,"keys":[{"key":"EOS5zASvvpQj31GZB4drkup2Kd7Z8wA1Gb9Pjwrdkr3kWLxoHW57p","weight":1}],"accounts":[{"permission":{"actor":"boxlendboxgv","permission":"eosio.code"},"weight":1}]}' \
-p boxlendboxgv@active

cleos -u http://n45i953228.qicp.vip set account permission boxgvlp22222 \
active '{"threshold":1,"keys":[{"key":"EOS7tufG7LXKqtFFSueMZ3PVyWa46xJNSBDLn7bzv9UtyQDUFnURy","weight":1}],"accounts":[{"permission":{"actor":"boxlendboxgv","permission":"eosio.code"},"weight":1}]}' \
-p boxgvlp22222@active

cleos -u http://n45i953228.qicp.vip push action crabfarm1111 \
add '[["0,BOXGV","lptoken.defi"],"boxgvlp22222",100, true]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action boxgvlp22222 \
init '[["0,BOXGV","lptoken.defi"],["4,USN","danchortoken"],["6,BOX","token.defi"],["6,BOX","token.defi"],"204","204","crabfarm1111",[100,200,200,10,0],43200]' -p devpcrabfarm@active

cleos -u https://eospush.tokenpocket.pro push action crabfarm1111 \
setenabled '[1, true]' -p devpcrabfarm@active