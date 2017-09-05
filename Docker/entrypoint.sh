#!/bin/bash
cd /opt/eos/bin

if [ -f '/opt/eos/bin/eosd-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/eos/bin/eosd-dir
fi
if [ -f '/opt/eos/bin/walletd-dir/config.ini' ]; then 
    echo
  else
    cp /walletd_config.ini /opt/eos/bin/walletd-dir/config.ini
fi

if [ -f '/opt/eos/bin/eosd-dir/genesis.json' ]; then
    echo
  else
    cp /genesis.json /opt/eos/bin/eosd-dir
fi

if [ -d '/opt/eos/bin/eosd-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/eos/bin/eosd-dir
fi
if [ -d '/opt/eos/bin/walletd-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/eos/bin/walletd-dir
fi
if [[ $EOS_APP = "walletd" ]]; then
  exec /opt/eos/bin/eos-walletd --data-dir /opt/eos/bin/walletd-dir $@
else
  exec /opt/eos/bin/eosd --data-dir /opt/eos/bin/eosd-dir $@
fi
