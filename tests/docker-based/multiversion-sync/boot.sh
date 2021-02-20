#!/bin/bash
set -ex

BIOS_ENDPOINT=http://bios:8888

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
cleos set contract eosio unittests/contracts/old_versions/v1.7.0-develop-preactivate_feature/eosio.bios

# Preactivate all digests
for digest in $FEATURE_DIGESTS;
do
   cleos push action eosio activate "{\"feature_digest\":\"$digest\"}" -p eosio
done

readarray syskey <<< $(cleos create key --to-console)
pubsyskey=${syskey[1]#"Public key: "}
prisyskey=${syskey[0]#"Private key: "}
cleos wallet import -n ignition --private-key $prisyskey

cleos create account eosio eosio.bpay $pubsyskey $pubsyskey
cleos create account eosio eosio.msig $pubsyskey $pubsyskey
cleos create account eosio eosio.names $pubsyskey $pubsyskey
cleos create account eosio eosio.ram $pubsyskey $pubsyskey
cleos create account eosio eosio.ramfee $pubsyskey $pubsyskey
cleos create account eosio eosio.saving $pubsyskey $pubsyskey
cleos create account eosio eosio.stake $pubsyskey $pubsyskey
cleos create account eosio eosio.token $pubsyskey $pubsyskey
cleos create account eosio eosio.vpay $pubsyskey $pubsyskey
cleos create account eosio eosio.wrap $pubsyskey $pubsyskey

cleos set contract eosio.token unittests/contracts/eosio.token eosio.token.wasm eosio.token.abi
cleos set contract eosio.msig unittests/contracts/eosio.msig eosio.msig.wasm eosio.msig.abi
cleos set contract eosio.wrap unittests/contracts/eosio.wrap eosio.wrap.wasm eosio.wrap.abi

cleos push action eosio.token create '["eosio","10000000000.0000 SYS"]' -p eosio.token
cleos push action eosio.token issue '["eosio","1000000000.0000 SYS","memo"]' -p eosio

# cleos set contract eosio unittests/contracts/eosio.system eosio.system.wasm eosio.system.abi
# cleos push action eosio init '{"version":"0","core":"4,SYS"}' --permission eosio@active

sleep 10
producer_objects=()
for producer in `ls /etc/eosio/producers`;
do 
  producer_objects+="{\"producer_name\": \"$producer\",\"block_signing_key\": \"$(cat /etc/eosio/producers/$producer)\"}"
done
IFS=, eval 'joined="${producer_objects[*]}"'   
cleos push action eosio setprods "{ \"schedule\": [$joined]}" -p eosio@active


