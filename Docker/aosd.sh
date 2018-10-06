#!/bin/sh
cd /opt/arisen/bin

if [ ! -d "/opt/arisen/bin/data-dir" ]; then
    mkdir /opt/arisen/bin/data-dir
fi

if [ -f '/opt/arisen/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/arisen/bin/data-dir
fi

if [ -d '/opt/arisen/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/arisen/bin/data-dir
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
    CONFIG_DIR="--config-dir=/opt/arisen/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/arisen/bin/aos $CONFIG_DIR "$@"
