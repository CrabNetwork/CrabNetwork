#!/bin/bash



cleos -u https://eospush.tokenpocket.pro push action launchpadddd clear '[1]' -p wdogdeployer

cleos -u https://eospush.tokenpocket.pro push action launchpadddd setwhitepair '[1, "100.0000 EOS", 14]' -p wdogdeployer
cleos -u https://eospush.tokenpocket.pro push action launchpadddd setwhitepair '[1, "100.0000 EOS", 28]' -p wdogdeployer
cleos -u https://eospush.tokenpocket.pro push action launchpadddd rmwhitepair '[1]' -p wdogdeployer
