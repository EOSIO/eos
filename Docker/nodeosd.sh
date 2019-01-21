#!/bin/sh
cd /opt/uosproject/bin

if [ ! -d "/opt/uosproject/bin/data-dir" ]; then
    mkdir /opt/uosproject/bin/data-dir
fi

if [ -f '/opt/uosproject/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/uosproject/bin/data-dir
fi

if [ -d '/opt/uosproject/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/uosproject/bin/data-dir
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
    CONFIG_DIR="--config-dir=/opt/uosproject/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/uosproject/bin/nodeos $CONFIG_DIR "$@"
