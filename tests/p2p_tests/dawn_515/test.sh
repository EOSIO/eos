#!/bin/bash

###############################################################
# Extracts staging directory and launchs cluster.
###############################################################

pnodes=1
total_nodes=3
topo=star
delay=1

read -d '' config00 << EOF
shared-file-size = 8192
p2p-server-address = localhost:9876
plugin = eosio::producer_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_plugin
plugin = eosio::account_history_api_plugin
required-participation = true
shared-file-dir = blockchain
http-server-address = 127.0.0.1:8888
block-log-dir = blocks
p2p-listen-endpoint = 0.0.0.0:9876
allowed-connection = any
private-key = ['EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV','5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3']
send-whole-blocks = true
readonly = 0
genesis-json = ./genesis.json
producer-name = inita
producer-name = initb
producer-name = initc
producer-name = initd
producer-name = inite
producer-name = initf
producer-name = initg
producer-name = inith
producer-name = initi
producer-name = initj
producer-name = initk
producer-name = initl
producer-name = initm
producer-name = initn
producer-name = inito
producer-name = initp
producer-name = initq
producer-name = initr
producer-name = inits
producer-name = initt
producer-name = initu
EOF

read -d '' config01 << EOF
genesis-json = ./genesis.json
block-log-dir = blocks
readonly = 0
send-whole-blocks = true
shared-file-dir = blockchain
shared-file-size = 8192
http-server-address = 127.0.0.1:8889
p2p-listen-endpoint = 0.0.0.0:9877
p2p-server-address = localhost:9877
allowed-connection = any
p2p-peer-address = localhost:9876
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_plugin
plugin = eosio::account_history_api_plugin
EOF

read -d '' config02 << EOF
genesis-json = ./genesis.json
block-log-dir = blocks
readonly = 0
send-whole-blocks = true
shared-file-dir = blockchain
shared-file-size = 8192
http-server-address = 127.0.0.1:8890
p2p-listen-endpoint = 0.0.0.0:9878
p2p-server-address = localhost:9877
allowed-connection = any
p2p-peer-address = localhost:9876
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_plugin
plugin = eosio::account_history_api_plugin
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

read -d '' logging02 << EOF
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
rm -rf tn_data_*

cName=config.ini
lName=logging.json

path=staging/tn_data_00
mkdir -p $path
echo "$config00" > $path/$cName
echo "$logging00" > $path/$lName

path=staging/tn_data_01
mkdir -p $path
echo "$config01" > $path/$cName
echo "$logging01" > $path/$lName

path=staging/tn_data_02
mkdir -p $path
echo "$config02" > $path/$cName
echo "$logging02" > $path/$lName


programs/launcher/launcher -p $pnodes -n $total_nodes --nogen -d $delay

sleep 1
res=$(grep "reason = duplicate" tn_data_0*/stderr.txt | wc -l)

if [ $res -ne 0 ]; then
    echo FAILURE: got a \"duplicate\" message
else
    echo "SUCCESS"
fi

programs/launcher/launcher -k 15
rm -rf staging
rm -rf tn_data_*
exit $res
