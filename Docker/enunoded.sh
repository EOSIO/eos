#!/bin/sh
cd /opt/enumivo/bin

if [ -f '/opt/enumivo/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/enumivo/bin/data-dir
fi

if [ -d '/opt/enumivo/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/enumivo/bin/data-dir
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
    CONFIG_DIR="--config-dir=/opt/enumivo/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/enumivo/bin/enunode $CONFIG_DIR $@
