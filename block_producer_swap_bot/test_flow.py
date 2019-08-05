import random
import subprocess
import time
from _sha256 import sha256
from datetime import datetime
from pprint import pprint

import requests
import web3
from eosiopy import sign
from eth_abi import encode_abi
from eth_account import Account
from web3 import Web3
from web3.middleware import geth_poa_middleware
from datetime import timezone

eth_provider = 'wss://rinkeby.infura.io/ws/v3/3f98ae6029094659ac8f57f66e673129'
eth_http_provider = 'https://rinkeby.infura.io/v3/3f98ae6029094659ac8f57f66e673129'
eth_chain_id = '0000000000000000000000000000000000000000000000000000000000000001'

gas_limit = 1000000
gas_price = 20000000000

eth_swap_contract_address = '0x819057C154e5B8C6b1c2baFE3c6ba57d04138345'
eth_rem_token_address = '0xdDB7456b6d76b6F17080439c98BB8Dad8B5Bae98'
decimals = 4

eth_rem_token_abi = '[ { "constant": true, "inputs": [], "name": "name", "outputs": [ { "name": "", "type": "string" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_spender", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "approve", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "totalSupply", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_from", "type": "address" }, { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "transferFrom", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "decimals", "outputs": [ { "name": "", "type": "uint8" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "issueTokens", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "_owner", "type": "address" } ], "name": "balanceOf", "outputs": [ { "name": "balance", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "symbol", "outputs": [ { "name": "", "type": "string" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "transfer", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "_owner", "type": "address" }, { "name": "_spender", "type": "address" } ], "name": "allowance", "outputs": [ { "name": "remaining", "type": "uint256" } ], "payable": false, "type": "function" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "from", "type": "address" }, { "indexed": true, "name": "to", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Transfer", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" }, { "indexed": true, "name": "spender", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Approval", "type": "event" } ]'
eth_swap_contract_abi = '[ { "constant": true, "inputs": [ { "name": "", "type": "uint256" } ], "name": "owners", "outputs": [ { "name": "", "type": "address" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" } ], "name": "removeOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "revokeConfirmation", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "REMCHAIN_ID", "outputs": [ { "name": "", "type": "bytes32" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "address" } ], "name": "isOwner", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "chainId", "type": "bytes32" }, { "name": "swapPubkey", "type": "string" }, { "name": "amountToSwap", "type": "uint256" } ], "name": "requestSwap", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "uint256" }, { "name": "", "type": "address" } ], "name": "confirmations", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "pending", "type": "bool" }, { "name": "executed", "type": "bool" } ], "name": "getTransactionCount", "outputs": [ { "name": "count", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" } ], "name": "addOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "isConfirmed", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "getConfirmationCount", "outputs": [ { "name": "count", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "MIN_SWAP_AMOUNT", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "uint256" } ], "name": "transactions", "outputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "data", "type": "bytes" }, { "name": "executed", "type": "bool" }, { "name": "swapId", "type": "bytes32" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "getOwners", "outputs": [ { "name": "", "type": "address[]" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "nonce", "type": "uint256" }, { "name": "data", "type": "bytes" } ], "name": "processSwapTransaction", "outputs": [ { "name": "transactionId", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "from", "type": "uint256" }, { "name": "to", "type": "uint256" }, { "name": "pending", "type": "bool" }, { "name": "executed", "type": "bool" } ], "name": "getTransactionIds", "outputs": [ { "name": "_transactionIds", "type": "uint256[]" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "getConfirmations", "outputs": [ { "name": "_confirmations", "type": "address[]" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "transactionCount", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_required", "type": "uint256" } ], "name": "changeRequirement", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "REMCHAIN_PUBKEY_LENGTH", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "confirmTransaction", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "data", "type": "bytes" } ], "name": "submitTransaction", "outputs": [ { "name": "transactionId", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "MAX_OWNER_COUNT", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "required", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" }, { "name": "newOwner", "type": "address" } ], "name": "replaceOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "executeTransaction", "outputs": [], "payable": false, "type": "function" }, { "inputs": [ { "name": "_owners", "type": "address[]" }, { "name": "_required", "type": "uint256" } ], "payable": false, "type": "constructor" }, { "payable": true, "type": "fallback" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Confirmation", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Revocation", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Submission", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Execution", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "ExecutionFailure", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Deposit", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" } ], "name": "OwnerAddition", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" } ], "name": "OwnerRemoval", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "required", "type": "uint256" } ], "name": "RequirementChange", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "sender", "type": "address" }, { "indexed": false, "name": "chainId", "type": "bytes32" }, { "indexed": false, "name": "swapPubkey", "type": "string" }, { "indexed": false, "name": "amountToSwap", "type": "uint256" }, { "indexed": false, "name": "timestamp", "type": "uint256" } ], "name": "SwapRequest", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "minSwapAmount", "type": "uint256" } ], "name": "MinSwapAmountChange", "type": "event" } ]'

amount_to_swap = 25

eth_address = '0x9f21F19180C8692EBaa061fd231cd1B029Ff2326'
eth_private_key = '3825e93a7999d3c800e62d98016b11b84d9ee10b9849a27cc1905ddf6521f13d'

remchain_sender_auth = 'useraaaaaaaa@active'

swap_contract_auth = 'useraaaaaaaa@active'
swap_contract_name = swap_contract_auth.split('@')[0]
contract_pubkey = 'EOS69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D'
producers = [
    'producer111a@active',
    'producer111c@active',
    # 'producer111c@active',
    'useraaaaaaaa@active',
]
txid = '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7'
remchain_id = '1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32'
swap_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
swap_privkey = '5JGzrjkwagH3MXvMvBdfWPzUrPEwSqrtNCXKdzzbtzjkhcbSjLc'
amount = f'{amount_to_swap}.0000 REM'
block_timestamp = '2019-07-28T20:51:47'
# block_timestamp = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
nodeos_url = 'http://165.22.209.37:8000'
wallet_url = 'http://165.22.209.37:6666'
cleos = f'cleos --url {nodeos_url} --wallet-url {wallet_url}'
get_actions_endpoint = '/v1/history/get_actions'

sender_auth = 'useraaaaaaaap@active'
receiver = 'mynewaccouat'
active_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
owner_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'

#contract_dir = '~/eosio.contracts/build/contracts'
swap_contract_path = '~/eos/tutorials/bios-boot-tutorial/remio.swap'


def get_timestamp(txid):
    w3 = Web3(Web3.HTTPProvider(eth_http_provider))
    w3.middleware_stack.inject(geth_poa_middleware, layer=0)
    block_num = w3.eth.getTransaction(txid)['blockNumber']
    return datetime.utcfromtimestamp(w3.eth.getBlock(block_num).timestamp).strftime("%Y-%m-%dT%H:%M:%S")


def step_issue_tokens():
    web3 = Web3(Web3.WebsocketProvider(eth_provider))

    data = data = b'GZ\x9f\xa9' + encode_abi(['address', 'uint256'], [eth_address, amount_to_swap*10^decimals])
    transaction = {
        'to': eth_rem_token_address,
        'value': 0,
        'gas': gas_limit,
        'gasPrice': gas_price,
        'nonce': web3.eth.getTransactionCount(eth_address),
        'data': data,
    }
    signed_tx = web3.eth.account.signTransaction(transaction, eth_private_key)
    return web3.eth.sendRawTransaction(signed_tx.rawTransaction).hex()


def step_approve():
    web3 = Web3(Web3.WebsocketProvider(eth_provider))

    data = b'\t^\xa7\xb3' + encode_abi(['address', 'uint256'], [eth_swap_contract_address, amount_to_swap*10**decimals])
    transaction = {
        'to': eth_rem_token_address,
        'value': 0,
        'gas': gas_limit,
        'gasPrice': gas_price,
        'nonce': web3.eth.getTransactionCount(eth_address),
        'data': data,
    }
    signed_tx = web3.eth.account.signTransaction(transaction, eth_private_key)
    return web3.eth.sendRawTransaction(signed_tx.rawTransaction).hex()


def step_request_swap():
    web3 = Web3(Web3.WebsocketProvider(eth_provider))

    data = b'0-%.' + encode_abi(['bytes32', 'string', 'uint256'], [bytes.fromhex(remchain_id), swap_pubkey[3:], amount_to_swap*(10**decimals)])
    transaction = {
        'to': eth_swap_contract_address,
        'value': 0,
        'gas': gas_limit,
        'gasPrice': gas_price,
        'nonce': web3.eth.getTransactionCount(eth_address),
        'data': data,
    }
    signed_tx = web3.eth.account.signTransaction(transaction, eth_private_key)
    return web3.eth.sendRawTransaction(signed_tx.rawTransaction).hex()


def step_set_contract():
    command = f'{cleos} set contract {swap_contract_name} {swap_contract_path} -p {swap_contract_auth}'
    subprocess.call(command, shell=True)


def step_update_auth():
    command = f"""
    {cleos} set account permission {swap_contract_name} active '{{"threshold": 1,"keys":
    [{{"key": "{contract_pubkey}","weight": 1}}],"accounts":
    [{{"permission":{{"actor":"{swap_contract_name}","permission":"eosio.code"}},"weight":1}}]}}' -p {swap_contract_auth}
    """
    subprocess.call(command, shell=True)


def step_process_swap():
    for prod_auth in producers:
        prod = prod_auth.split('@')[0]
        command = f"""
        {cleos} push action {swap_contract_name} processswap '[ "{prod}",
        "{txid}",
        "{remchain_id}",
        "{swap_pubkey}", "{amount}",
        "{block_timestamp}" ]' -p {prod_auth}
        """
        subprocess.call(command, shell=True)


def step_finish_swap():
    if active_pubkey != '':
        swap_id_str = (receiver + "*" + active_pubkey[3:] + "*" + owner_pubkey[3:]).encode()
    else:
        swap_id_str = receiver.encode()
    print(swap_id_str)
    sig = sign(swap_privkey, swap_id_str)
    command = f"""
    {cleos} push action {swap_contract_name} finishswap '[ "{receiver}",
    "{txid}",
    "{remchain_id}",
    "{swap_pubkey}",
    "{amount}", "{block_timestamp}",
    "{sig}",
    "{active_pubkey}",
    "{owner_pubkey}" ]' -p {sender_auth}

    """
    return subprocess.call(command, shell=True)


def check_swap_result():
    command = f"""
    {cleos} get account {receiver}
    """
    subprocess.call(command, shell=True)


def step_transfer_to_remio_swap():
    remchain_sender_address = remchain_sender_auth.split('@')[0]
    command = f"""
    {cleos} push action eosio.token transfer '["{remchain_sender_address}",
    "{swap_contract_name}","{amount_to_swap}.0000 REM","{eth_chain_id} {eth_address}"]' -p {remchain_sender_auth}
    """
    print(command)

    subprocess.call(command, shell=True)


def step_transfer_to_remio_swap_invalid_memo():
    remchain_sender_address = remchain_sender_auth.split('@')[0]
    command = f"""
    {cleos} push action eosio.token transfer '["{remchain_sender_address}",
    "{swap_contract_name}","{amount_to_swap}.0000 REM","INVALID MEMO"]' -p {remchain_sender_auth}
    """
    print(command)

    subprocess.call(command, shell=True)


def get_transfer_actions():
    payload = f"{{\"pos\":\"0\",\"offset\":\"1000\",\"account_name\":\"{swap_contract_name}\"}}"
    headers = {'content-type': 'application/json'}
    response = requests.request("POST", nodeos_url+get_actions_endpoint, data=payload, headers=headers)

    pprint(response.text)


def process_swap_to_eth():

    bot_private_keys = [
        '3825e93a7999d3c800e62d98016b11b84d9ee10b9849a27cc1905ddf6521f13d',
        'd77f95aac9583a0a1980a08fdd1e2d018d9033b432ebb1c1b13aa6a0086f3e63',
        'e970407bd61bcb3bcaa678dbfe4a93ec5f6fcc087926987c56aa56705cbbd782',
    ]
    quantity = 25 * 10 ** decimals
    tx_ids = []
    remchain_nonce = random.randint(1, 1_000_000)
    web3 = Web3(Web3.HTTPProvider(eth_http_provider))

    for bot_private_key in bot_private_keys:
        bot_account = Account.privateKeyToAccount(bot_private_key)
        bot_address = bot_account.address
        bot_nonce = web3.eth.getTransactionCount(bot_address)

        tx_data = b'\xa9\x05\x9c\xbb' + \
                  encode_abi(['address', 'uint'], [eth_address, quantity])

        data = b'\xa3\xc9\xeaz' + encode_abi(['address', 'uint', 'uint', 'bytes'],
                                             [eth_rem_token_address, 0, remchain_nonce, tx_data])
        transaction = {
            'to': eth_swap_contract_address,
            'value': 0,
            'gas': gas_limit,
            'gasPrice': gas_price,
            'nonce': bot_nonce,
            'data': data,
        }
        signed_tx = web3.eth.account.signTransaction(transaction, bot_private_key)
        txid = web3.eth.sendRawTransaction(signed_tx.rawTransaction).hex()
        if txid:
            tx_ids.append(txid)
    return tx_ids


def get_swap_id(txid, chain_id, swap_pubkey, amount, timestamp):
    epoch = datetime.utcfromtimestamp(0)
    block_datetime_utc = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S').replace(tzinfo=timezone.utc).timestamp()#datetime(2019, 7, 28, 20, 51, 47).replace(tzinfo=timezone.utc).timestamp()
    epoch_datetime_utc = datetime.utcfromtimestamp(0).replace(tzinfo=timezone.utc).timestamp()
    seconds_since_epoch = str(int(block_datetime_utc - epoch_datetime_utc))
    print(seconds_since_epoch)
    swap_str = txid + "*" + chain_id + "*" + swap_pubkey[3:] + "*" + amount + "*" + seconds_since_epoch
    print(swap_str)
    return sha256(swap_str.encode()).hexdigest()


if __name__ == '__main__':
    # print('issue: ', step_issue_tokens())
    # time.sleep(25)
    # print('approve: ', step_approve())
    # time.sleep(25)
    # txid = step_request_swap()
    # print('request swap: ', step_request_swap())
    # time.sleep(25)
    # block_timestamp = get_timestamp(txid)
    #print(process_swap_to_eth())
     step_set_contract()
     step_update_auth()
    #step_process_swap()
    #step_transfer_to_remio_swap_invalid_memo()
    #get_transfer_actions()
    # finished = False
    # while not finished:
    #     finished = (step_finish_swap() == 0)
    #     time.sleep(10)
    # check_swap_result()
