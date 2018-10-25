#!/bin/sh
cd /opt/cyberway/bin

if [ ! -d "/opt/cyberway/bin/data-dir" ]; then
    mkdir /opt/cyberway/bin/data-dir
fi

if [ -f '/opt/cyberway/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/cyberway/bin/data-dir
fi

if [ -d '/opt/cyberway/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/cyberway/bin/data-dir
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
    CONFIG_DIR="--config-dir=/opt/cyberway/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/cyberway/bin/nodeos $CONFIG_DIR "$@"
