#!/bin/bash
set -ex

BIOS_ENDPOINT=http://bios:8888

producer_name=${HOSTNAME}


function cleos {
   /usr/local/bin/cleos --url $BIOS_ENDPOINT "${@}"
}

function wait_bios_ready {
  for (( i=0 ; i<10; i++ )); do
    ! cleos get info || break
    sleep 3
  done
}

cd /root/eosio
wait_bios_ready

readarray syskey <<< $(cleos create key --to-console)
pub_key=`echo -n ${syskey[1]#"Public key: "}`
pri_key=`echo -n ${syskey[0]#"Private key: "}`

echo $pub_key > /etc/eosio/producers/$producer_name

cleos wallet create --to-console
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos create account eosio $producer_name ${pub_key} ${pub_key}

for host in ${peers//;/ }; do
  peer_args="$peer_args --p2p-peer-address $host"
done

nodeos --producer-name $producer_name --signature-provider ${pub_key}=KEY:${pri_key} $peer_args "${@}"