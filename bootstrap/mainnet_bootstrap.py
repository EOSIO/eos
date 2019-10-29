import json
import subprocess
from time import sleep

import requests

system_accounts = [
    'rem.bpay',
    'rem.msig',
    'rem.names',
    'rem.ram',
    'rem.ramfee',
    'rem.saving',
    'rem.stake',
    'rem.token',
    'rem.vpay',
    'rem.spay',
    'rem.rex',
    'rem.swap',
    'rem.oracle',
    'rem.utils',
    'rem.auth',
    'rem.attr',
]

remcode_permission_accounts = [
    'rem.swap',
    'rem.oracle',
    'rem.utils',
    'rem.auth',
    'rem.attr',
]

tech_accounts = {
    'swapbot': 'EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr',
}

remnode_port = 8888
wallet_port = 6666
remcli = f'remcli --url http://127.0.0.1:{remnode_port} --wallet-url http://127.0.0.1:{wallet_port} '

public_key = 'EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr'

contracts_dir = '../build/contracts/contracts'
eosio_contracts_dir = '../unittests/contracts/old_versions/v1.7.0-develop-preactivate_feature'
max_rem_supply = 1_000_000_000_0000
max_auth_token_supply = 1_000_000_000_000_0000
rem_symbol = 'REM'
auth_symbol = 'AUTH'

producer_reward_per_swap = 10_0000  # torewards 10.0000 REM per swap
min_swap_out_amount = 100_0000

swap_chains = [
    # (chain_id, input, output)
    ('ethropsten', '1', '1')
]


def get_chain_id():
    url = f'http://127.0.0.1:{remnode_port}/v1/chain/get_info'
    headers = {'accept': 'application/json'}
    response = requests.request("POST", url, headers=headers)
    return response.json()['chain_id']


def run(args):
    print('mainnet_bootstrap.py:', args)
    if subprocess.call(args, shell=True):
        print('mainnet_bootstrap.py: error')


def retry(args):
    while True:
        print('mainnet_bootstrap.py:', args)
        if subprocess.call(args, shell=True):
            print('*** Retry')
        else:
            break


def jsonArg(a):
    return " '" + json.dumps(a) + "' "


def intToRemCurrency(i):
    return '%d.%04d %s' % (i // 10000, i % 10000, rem_symbol)


def intToAuthCurrency(i):
    return '%d.%04d %s' % (i // 10000, i % 10000, auth_symbol)


def activate_protocol_features():
    run(f'curl -X POST http://127.0.0.1:{remnode_port}/v1/producer/schedule_protocol_feature_activations -d \'{{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}}\'')


def create_system_accounts():
    for a in system_accounts:
        run(remcli + 'create account rem ' + a + ' ' + public_key)


def create_tech_accounts():
    for account_name, public_key in tech_accounts.items():
        run(remcli + 'create account rem ' + account_name + ' ' + public_key)


def configure_swapbot_permissions():
    run(remcli + 'set account permission swapbot bot ' + jsonArg({
        'threshold': 1,
        'keys': [{"key": tech_accounts['swapbot'], "weight": 1}],
    }) + "active")
    run(remcli + 'push action rem linkauth ' + jsonArg({
        'account': 'swapbot',
        'code': 'rem.swap',
        'type': 'finish',
        'requirement': 'bot',
    }) + " -p swapbot@active")
    run(remcli + 'push action rem linkauth ' + jsonArg({
        'account': 'swapbot',
        'code': 'rem.swap',
        'type': 'finishnewacc',
        'requirement': 'bot',
    }) + " -p swapbot@active")


def configure_remcode_permissions():
    for account_name in remcode_permission_accounts:
        run(remcli + f'set account permission {account_name} active ' + jsonArg({
            'threshold': 1,
            'keys': [{"key": public_key, "weight": 1}],
            'accounts': [{"permission": {"actor": account_name, "permission": "rem.code"}, "weight": 1}]
        }) + "owner")


def install_system_contracts():
    run(remcli + 'set contract rem.token ' + contracts_dir + '/rem.token/')
    run(remcli + 'set contract rem.msig ' + contracts_dir + '/rem.msig/')
    run(remcli + 'set contract rem.swap ' + contracts_dir + '/rem.swap/')
    run(remcli + 'set contract rem.oracle ' + contracts_dir + '/rem.oracle/')
    run(remcli + 'set contract rem.utils ' + contracts_dir + '/rem.utils/')
    run(remcli + 'set contract rem.auth ' + contracts_dir + '/rem.auth/')
    run(remcli + 'set contract rem.attr ' + contracts_dir + '/rem.attr/')


def bootstrap_system_contracts():
    run(remcli + 'push action rem.oracle addpair \'["rem.usd"]\' -p rem.oracle -p rem')
    run(remcli + 'push action rem.oracle addpair \'["rem.btc"]\' -p rem.oracle -p rem')
    run(remcli + 'push action rem.oracle addpair \'["rem.eth"]\' -p rem.oracle -p rem')

    run(remcli + f'push action rem.swap setswapfee \'["{producer_reward_per_swap}"]\' \
    -p rem.swap -p rem')
    run(remcli + f'push action rem.swap setminswpout \'["{min_swap_out_amount}"]\' \
    -p rem.swap -p rem')
    run(remcli + f'push action rem.swap setchainid \'["{get_chain_id()}"]\' \
    -p rem.swap -p rem')
    for chain_id, input, output, in swap_chains:
        run(remcli + f'push action rem.swap addchain \'["{chain_id}", "{input}", "{output}"]\' \
        -p rem.swap -p rem')


def create_rem_token():
    run(remcli + 'push action rem.token create \'["rem.swap", "%s"]\' -p rem.token' % intToRemCurrency(max_rem_supply))
    run(remcli + 'push action rem.token issue \'["rem.swap", "%s", "memo"]\' -p rem.swap' % intToRemCurrency(1))


def create_auth_token():
    run(remcli + 'push action rem.token create \'["rem.auth", "%s"]\' -p rem.token'
        % intToAuthCurrency(max_auth_token_supply))


def set_system_contract():
    run(remcli + 'set contract rem ' + eosio_contracts_dir + '/eosio.bios/ -p rem')
    run(
        remcli + 'push action rem activate \'["299dcb6af692324b899b39f16d5a530a33062804e41f09dc97e9f156b4476707"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["4fca8bd82bbd181e714e283f83e1b45d95ca5af40fb89ad3977b653c448f78c2"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241"]\' -p rem')
    run(
        remcli + 'push action rem activate \'["4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67"]\' -p rem')
    run(remcli + 'set contract rem ' + contracts_dir + '/rem.bios/ -p rem')
    retry(remcli + 'set contract rem ' + contracts_dir + '/rem.system/')
    sleep(1)
    run(remcli + 'push action rem init' + jsonArg(['0', '4,' + rem_symbol]) + '-p rem@active')
    sleep(1)
    run(remcli + 'push action rem setpriv' + jsonArg(['rem.msig', 1]) + '-p rem@active')


if __name__ == '__main__':
    activate_protocol_features()
    create_system_accounts()
    create_tech_accounts()
    configure_swapbot_permissions()
    install_system_contracts()
    configure_remcode_permissions()
    create_rem_token()
    create_auth_token()
    set_system_contract()
    bootstrap_system_contracts()
