#!/bin/sh
cd /opt/eosio/bin
EOSIOBIN="/opt/eosio/bin"
# Set Genesis if the blocks directory does not exist yet
if [ ! -d "${EOSIOBIN}/data-dir/blocks" ]; then
	GENESIS="--genesis-json ${EOSIOBIN}/data-dir/genesis.json"
fi

if [ -f "${EOSIOBIN}/data-dir/config.ini" ]; then
    echo
  else
    cp /config.ini $EOSIOBIN/data-dir
fi

if [ -d "${EOSIOBIN}/data-dir/contracts" ]; then
    echo
  else
    cp -r /contracts $EOSIOBIN/data-dir
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

if [ ! "${CONFIG_DIR}" ]; then
    CONFIG_DIR="--config-dir="${EOSIOBIN}/data-dir"
else
    CONFIG_DIR=""
fi

exec $EOSIOBIN/nodeos $CONFIG_DIR $GENESIS "$@"

