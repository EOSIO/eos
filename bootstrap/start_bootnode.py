import os
import subprocess
from mainnet_bootstrap import run, remcli, public_key
from time import sleep

wallet_dir = './wallet/'
remvault = 'remvault '
remnode = 'remnode '
nodes_dir = './nodes/'
genesis = './genesis.json'
unlockTimeout = 999999999
maxClients = 30

cryptocompare_apikey = '3149f5165071436773542056dc30d8ec7d4ba986968c51e46103c19a212a21ed'
eth_wss_provider = 'wss://ropsten.infura.io/ws/v3/3f98ae6029094659ac8f57f66e673129'

private_key = '5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p'


def kill_all():
    run('killall remvault remnode || true')
    sleep(1.5)


def background(args):
    print('start_bootnode.py:', args)
    return subprocess.Popen(args, shell=True)


def start_wallet():
    run('rm -rf ' + os.path.abspath(wallet_dir))
    run('mkdir -p ' + os.path.abspath(wallet_dir))
    background(remvault + ' --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s'\
               % (unlockTimeout, os.path.abspath(wallet_dir)))
    sleep(.4)
    run(remcli + 'wallet create --to-console')


def import_private_keys():
    run(remcli + 'wallet import --private-key ' + private_key)


def start_node(nodeIndex, account):
    dir = nodes_dir + ('%02d-' % nodeIndex) + account['name'] + '/'
    run('rm -rf ' + dir)
    run('mkdir -p ' + dir)
    otherOpts = ''.join(list(map(lambda i: '    --p2p-peer-address localhost:' + str(9000 + i), range(nodeIndex))))
    if not nodeIndex: otherOpts += (
        '    --plugin eosio::history_plugin'
        '    --plugin eosio::history_api_plugin'
    )
    swap_and_oracle_opts = (
                '    --plugin eosio::eth_swap_plugin'
                '    --plugin eosio::rem_oracle_plugin'
                '    --oracle-authority ' + account['name'] + '@active' +
                '    --oracle-signing-key ' + account['pvt'] +
                '    --swap-signing-key ' + account['pvt'] +
                '    --swap-authority ' + account['name'] + '@active'
                '    --update_price_period 5'
        )
    if cryptocompare_apikey:
        swap_and_oracle_opts += (
            '    --cryptocompare-apikey ' + cryptocompare_apikey
        )
    if eth_wss_provider:
        swap_and_oracle_opts += (
            '    --eth-wss-provider ' + eth_wss_provider
        )
    cmd = (
        remnode +
        '    --max-irreversible-block-age -1'
        '    --contracts-console'
        '    --genesis-json ' + os.path.abspath(genesis) +
        '    --blocks-dir ' + os.path.abspath(dir) + '/blocks'
        '    --config-dir ' + os.path.abspath(dir) +
        '    --data-dir ' + os.path.abspath(dir) +
        '    --chain-state-db-size-mb 1024'
        '    --http-server-address 127.0.0.1:' + str(8888 + nodeIndex) +
        '    --p2p-listen-endpoint 127.0.0.1:' + str(9000 + nodeIndex) +
        '    --max-clients ' + str(maxClients) +
        '    --p2p-max-nodes-per-host ' + str(maxClients) +
        '    --enable-stale-production'
        '    --producer-name ' + account['name'] +
        '    --private-key \'["' + account['pub'] + '","' + account['pvt'] + '"]\''
        '    --access-control-allow-origin=\'*\''
        '    --plugin eosio::http_plugin'
        '    --plugin eosio::chain_api_plugin'
        '    --plugin eosio::producer_api_plugin'
        '    --plugin eosio::producer_plugin' + swap_and_oracle_opts +
        otherOpts)
    with open(dir + 'stderr', mode='w') as f:
        f.write(cmd + '\n\n')
    background(cmd + '    2>>' + dir + 'stderr')
    sleep(2)
    run(f'curl -X POST http://127.0.0.1:{8888+nodeIndex}/v1/producer/schedule_protocol_feature_activations -d \'{{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}}\'')


def step_start_boot():
    start_node(0, {'name': 'rem', 'pvt': private_key, 'pub': public_key})
    sleep(1.5)


if __name__ == '__main__':
    kill_all()
    start_wallet()
    import_private_keys()
    step_start_boot()

