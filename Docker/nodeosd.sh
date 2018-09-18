#!/bin/sh
cd /opt/eosio/bin

if [ ! -d "/opt/eosio/bin/data-dir" ]; then
    mkdir /opt/eosio/bin/data-dir
fi

if [ -f '/opt/eosio/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/eosio/bin/data-dir
fi

if [ -d '/opt/eosio/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/eosio/bin/data-dir
fi

while :; do
    case $1 in
        --config-dir=?*)
            CONFIG_DIR=${1#*=}
            ;;
        *)
            break
    esac
    shift
done

if [ ! "$CONFIG_DIR" ]; then
    CONFIG_DIR="--config-dir=/opt/eosio/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/eosio/bin/nodeos $CONFIG_DIR "$@"
