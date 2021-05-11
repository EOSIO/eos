#!/bin/bash
set -ex

BIOS_ENDPOINT=http://producer:8888

function cleos {
   /usr/local/bin/cleos --url $BIOS_ENDPOINT "${@}"
}

function wait_bios_ready {
  for (( i=0 ; i<10; i++ )); do
    ! cleos get info || break
    sleep 3
  done
}

wait_bios_ready

cd /root/eosio

cleos wallet create --to-console -n ignition
cleos wallet import -n ignition --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
curl -X POST $BIOS_ENDPOINT/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}'
FEATURE_DIGESTS=`curl $BIOS_ENDPOINT/v1/producer/get_supported_protocol_features | jq -r -c 'map(select(.specification[].value | contains("PREACTIVATE_FEATURE") | not) | .feature_digest )[]'`
sleep 10
cleos set contract eosio contracts/eosio.boot

# Preactivate all digests
for digest in $FEATURE_DIGESTS;
do
   cleos push action eosio activate "{\"feature_digest\":\"$digest\"}" -p eosio
done
sleep 1s
cleos set contract eosio contracts/eosio.bios -p eosio@active
sleep 1s
cleos push action eosio setkvparams '[{"max_key_size":64, "max_value_size":1024, "max_iterators":128}]' -p eosio@active
sleep 1s
cleos set contract eosio contracts/kv_contract 
cleos push action eosio setkvparam '["eosio.kvram"]' --permission eosio
curl --data-binary '["eosio","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]' $BIOS_ENDPOINT/v1/txn_test_gen/create_test_accounts

readarray syskey <<< $(cleos create key --to-console)
pubsyskey=${syskey[1]#"Public key: "}
prisyskey=${syskey[0]#"Private key: "}
cleos wallet import -n ignition --private-key $prisyskey
cleos create account eosio txn.test.b $pubsyskey $pubsyskey
cleos set contract txn.test.b contracts/kv_contract -p txn.test.b@active
curl --data-binary '["", 20, 20]' $BIOS_ENDPOINT/v1/txn_test_gen/start_generation
