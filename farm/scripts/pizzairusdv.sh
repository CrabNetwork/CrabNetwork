#/bin/bash

cleos -u http://eospush.tokenpocket.pro set account permission pizzairusdv1 \
active '{"threshold":1,"keys":[{"key":"EOS7tufG7LXKqtFFSueMZ3PVyWa46xJNSBDLn7bzv9UtyQDUFnURy","weight":1}],"accounts":[{"permission":{"actor":"pizzairusdv1","permission":"eosio.code"},"weight":1}]}' \
-p pizzairusdv1@active

cleos -u http://eospush.tokenpocket.pro push action crabfarm1111 \
add '[["4,USDV","lptoken.air"],"pizzairusdv1",1,true]' -p devpcrabfarm@active

cleos -u http://eospush.tokenpocket.pro push action pizzairusdv1 \
init '[["4,USDV","lptoken.air"],["4,USDT","tethertether"],["8,AIR","token.air"],["8,AIR","token.air"],"swap-USDV","swap-USDV","crabfarm1111",[100,200,200,10,0],43200]' -p devpcrabfarm@active