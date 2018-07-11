#!/bin/sh

if [ -f '/mnt/dev/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /mnt/dev/data-dir
fi

if [ -d '/mnt/dev/data-dir/contracts' ]; then
    echo
  else
    cp -r /eos/contracts /mnt/dev/data-dir
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
    CONFIG_DIR="--config-dir=/mnt/dev/data-dir"
else
    CONFIG_DIR=""
fi

exec nodeos $CONFIG_DIR "$@"
