#!/usr/bin/bash -x
# Test for amqp queue listening to be paused in a paused producer

echo 'making sure we are clean.  one or both of these commands may fail.'
killall nodeos
rm -rf var/lib/node_004

echo 'starting producer and sending txns'
tests/nodeos_high_transaction_test.py -p=1 -n=8 --dump-error-details --keep-logs -v --leave-running --amqp-address=amqp://guest:guest@127.0.0.1 --num-transactions=90000000000 > var/lib/amqp.txt 2>&1 &
sleep 5

echo 'checking for amqp start up in main producer'
amqp_startup=`fgrep -e "Starting amqp consumption at startup"  var/lib/node_00/stderr.txt | wc -l`

if [ $amqp_startup != 1 ]; then
	echo 'Active producer did not start amqp consumpution'
	exit -1
fi

echo 'configuring backup'
cp -r   etc/eosio/node_00   etc/eosio/node_004


sed -i -e '/http-server-address/d'  etc/eosio/node_004/config.ini
sed -i -e '/p2p-listen-endpoint/d'  etc/eosio/node_004/config.ini
sed -i -e '/p2p-server-address/d'  etc/eosio/node_004/config.ini


echo "http-server-address = 127.0.0.1:8118
p2p-listen-endpoint = 0.0.0.0:98761
p2p-server-address = localhost:98761 " >> etc/eosio/node_004/config.ini


cat etc/eosio/node_004/config.ini 


mkdir -p var/lib/node_004
nodeos_pid=`cat var/lib/node_00/nodeos.pid`

genesis_timestamp=`grep -o -m 1 -e '--genesis-timestamp [^ ]*' var/lib/amqp.txt`

echo 'starting backup'
programs/nodeos/nodeos $genesis_timestamp --max-transaction-time -1 --abi-serializer-max-time-ms 990000 --p2p-max-nodes-per-host 8 --contracts-console --plugin eosio::producer_api_plugin --plugin eosio::amqp_trx_plugin --amqp-trx-address amqp://guest:guest@127.0.0.1  --amqp-trx-ack-mode=executed  --http-max-response-time-ms 990000 --config-dir etc/eosio/node_004 --data-dir var/lib/node_004 --genesis-json etc/eosio/node_00/genesis.json --pause-on-startup > var/lib/node_004/stderr.txt 2>&1 &

echo 'waiting for backup to start'
sleep 10 

echo 'checking that backup did not start consuming'
amqp_startup=`fgrep -e "Starting amqp consumption at startup"  var/lib/node_004/stderr.txt | wc -l`
if [ $amqp_startup != 0 ]; then
	echo 'Paused producer started amqp consumption'
	exit -1
fi

echo 'stopping main producer'
kill -s STOP $nodeos_pid

echo 'resuming backup producer'
curl --request POST --url http://127.0.0.1:8118/v1/producer/resume


echo 'checking for backup producer to start consuming'
sleep 5
amqp_startup=`fgrep -e "consuming amqp messages during on_block_start"  var/lib/node_004/stderr.txt | wc -l`
if [ $amqp_startup != 1 ]; then
	echo 'Resumed producer did not start amqp consumption'
	exit -1
fi


echo 'test ran successfully'
killall nodeos

exit 0 
