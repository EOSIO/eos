#!/bin/sh
cd /opt/eosio/bin

if [ ! -f '/opt/eosio/bin/data-dir/config.ini' ]; then
    cp /config.ini /opt/eosio/bin/data-dir
fi

if [ ! -f '/opt/eosio/bin/genesis.json' ]; then
    cp /genesis.json /opt/eosio/bin
fi

if [ ! -d '/opt/eosio/bin/data-dir/contracts' ]; then
    cp -r /contracts /opt/eosio/bin/data-dir
fi

exec /opt/eosio/bin/eosiod $@
