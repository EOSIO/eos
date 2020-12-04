## Summary
This README illustrates the steps for usage of enable-kv.sh script with kv_map as an example. Set your environment variables as follows before beginning the rest of the steps.
1. export EOS_3_0=[eos 3.0 directory]
1. export EOSIO_CDT_2_0=[eosio.cdt 2.0 directory]
1. export PATH=$EOS_3_0/build/bin:$PATH

## Steps
Bring up nodeos in a different terminal
1. export PATH=$EOS_3_0/build/bin:$PATH
1. nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin --plugin eosio::history_plugin --plugin eosio::history_api_plugin --filter-on=* --access-control-allow-origin=* --contracts-console --http-validate-host=false --verbose-http-errors --max-transaction-time=1000 --backing-store chainbase --data-dir=datadir

In the first terminal
1. $EOS_3_0/contracts/enable-kv/enable-kv.sh -c $EOS_3_0/build/contracts/contracts/
1. cd $EOSIO_CDT_2_0/examples/kv_map
1. mkdir build
1. cd build
1. cmake .. -DCMAKE_PREFIX_PATH=$EOS_3_0/build
1. make
1. cleos create account eosio jane EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
1. cleos set contract jane $EOSIO_CDT_2_0/examples/kv_map/build/kv_map -p jane@active
1. cleos push action jane upsert '[1, "jane.acct" "jane", "doe", "1 main st", "new york", "NY", "USA", "123"]' -p jane@active
1. cleos push action jane get '[1]' -p jane@active