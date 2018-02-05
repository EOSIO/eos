#!/bin/sh
cd /opt/eosio/bin

if [ -f '/opt/eosio/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/eosio/bin/data-dir
fi

if [ -f '/opt/eosio/bin/data-dir/genesis.json' ]; then
    echo
  else
    cp /genesis.json /opt/eosio/bin/data-dir
fi

if [ -d '/opt/eosio/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/eosio/bin/data-dir
fi

exec /opt/eosio/bin/eosiod $@
