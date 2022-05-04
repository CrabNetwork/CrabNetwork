#/bin/bash

cleos -u https://eospush.tokenpocket.pro set account permission crabfarm1111 \
active '{"threshold":1,"keys":[{"key":"EOS7cp6WdhwZZzmLDKF495K6wqo7DK7xn7K3xYyjn9AEdcym2TA28","weight":1}],"accounts":[{"permission":{"actor":"crabfarm1111","permission":"eosio.code"},"weight":1}]}' \
-p crabfarm1111@active

cleos -u https://eospush.tokenpocket.pro push action boxlendboxgv earn '[]' -p devpcrabfarm@active



#===== test ==========

cleos -u http://n45i953228.qicp.vip set account permission crabfarm1111 \
active '{"threshold":1,"keys":[{"key":"EOS5zASvvpQj31GZB4drkup2Kd7Z8wA1Gb9Pjwrdkr3kWLxoHW57p","weight":1}],"accounts":[{"permission":{"actor":"crabfarm1111","permission":"eosio.code"},"weight":1}]}' \
-p crabfarm1111@active



