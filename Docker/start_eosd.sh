#!/bin/sh
cd /opt/eos/bin

if [ -f '/opt/eos/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/eos/bin/data-dir
fi

if [ -f '/opt/eos/bin/data-dir/genesis.json' ]; then
    echo
  else
    cp /genesis.json /opt/eos/bin/data-dir
fi

if [ -d '/opt/eos/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/eos/bin/data-dir
fi

exec /opt/eos/bin/eosd $@
