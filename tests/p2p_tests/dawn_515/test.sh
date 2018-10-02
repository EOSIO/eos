#!/bin/bash

#
# This test ensures that connections with the same p2p-server-address
# are not closed as redundant.
#

###############################################################
# Extracts staging directory and launchs cluster.
###############################################################

pnodes=1
total_nodes=2
topo=star
delay=1

read -d '' genesis << EOF
{
  "initial_timestamp": "2018-06-01T12:00:00.000",
  "initial_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 200000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 150000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 4096,
    "max_inline_action_depth": 4,
    "max_authority_depth": 6
}
EOF

read -d '' configbios << EOF
p2p-server-address = localhost:9876
plugin = eosio::producer_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::net_plugin
plugin = eosio::history_api_plugin
http-server-address = 127.0.0.1:8888
blocks-dir = blocks
p2p-listen-endpoint = 0.0.0.0:9876
allowed-connection = any
private-key = ['EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV','5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3']
send-whole-blocks = true
readonly = 0
p2p-max-nodes-per-host = 10
enable-stale-production = true
producer-name = eosio
EOF

read -d '' config00 << EOF
blocks-dir = blocks
readonly = 0
send-whole-blocks = true
http-server-address = 127.0.0.1:8889
p2p-listen-endpoint = 0.0.0.0:9877
p2p-server-address = localhost:9877
allowed-connection = any
p2p-peer-address = localhost:9876
plugin = eosio::chain_api_plugin
EOF

read -d '' config01 << EOF
blocks-dir = blocks
readonly = 0
send-whole-blocks = true
http-server-address = 127.0.0.1:8890
p2p-listen-endpoint = 0.0.0.0:9878
p2p-server-address = localhost:9877
allowed-connection = any
p2p-peer-address = localhost:9876
plugin = eosio::chain_api_plugin
EOF

read -d '' loggingbios << EOF
{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    }
  ]
}
EOF

read -d '' logging00 << EOF
{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    }
  ]
}
EOF

read -d '' logging01 << EOF
{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr"
      ]
    }
  ]
}
EOF

rm -rf staging
rm -rf etc/eosio/node_*
rm -rf var/lib
cName=config.ini
lName=logging.json
gName=genesis.json

path=staging/etc/eosio/node_bios
mkdir -p $path
echo "$configbios" > $path/$cName
echo "$loggingbios" > $path/$lName
echo "$genesis" > $path/$gName

path=staging/etc/eosio/node_00
mkdir -p $path
echo "$config00" > $path/$cName
echo "$logging00" > $path/$lName
echo "$genesis" > $path/$gName

path=staging/etc/eosio/node_01
mkdir -p $path
echo "$config01" > $path/$cName
echo "$logging01" > $path/$lName
echo "$genesis" > $path/$gName


programs/eosio-launcher/eosio-launcher -p $pnodes -n $total_nodes --nogen -d $delay

sleep 5
res=$(grep "reason = duplicate" var/lib/node_*/stderr.txt | wc -l)
ret=0

if [ $res -ne 0 ]; then
    echo FAILURE: got a \"duplicate\" message
    ret=1
fi

b5idbios=`./programs/cleos/cleos -u http://localhost:8888 get block 5 | grep "^ *\"id\""`
b5id00=`./programs/cleos/cleos -u http://localhost:8889 get block 5 | grep "^ *\"id\""`
b5id01=`./programs/cleos/cleos -u http://localhost:8890 get block 5 | grep "^ *\"id\""`

if [ "$b5idbios" != "$b5id00" ]; then
    echo FAILURE: nodes are not in sync
    ret=1
fi

if [ "$b5idbios" != "$b5id01" ]; then
    echo FAILURE: nodes are not in sync
    ret=1
fi

if [ $ret  -eq 0 ]; then
    echo SUCCESS
fi

programs/eosio-launcher/eosio-launcher -k 15
rm -rf staging
rm -rf var/lib/node_*
rm -rf etc/eosio/node_*
exit $ret
