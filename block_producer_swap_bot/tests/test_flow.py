import subprocess
import time
from datetime import datetime

import web3
from eosiopy import sign
from eth_abi import encode_abi
from web3 import Web3
from web3.middleware import geth_poa_middleware

eth_provider = 'wss://rinkeby.infura.io/ws/v3/3f98ae6029094659ac8f57f66e673129'
eth_http_provider = 'https://rinkeby.infura.io/v3/3f98ae6029094659ac8f57f66e673129'

eth_swap_contract_address = '0xCEe6a3967cfbd2e72E6e51066508Ab2B961207d1'
eth_rem_token_address = '0xdDB7456b6d76b6F17080439c98BB8Dad8B5Bae98'
decimals = 4

eth_rem_token_abi = '[ { "constant": true, "inputs": [], "name": "name", "outputs": [ { "name": "", "type": "string" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_spender", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "approve", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "totalSupply", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_from", "type": "address" }, { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "transferFrom", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "decimals", "outputs": [ { "name": "", "type": "uint8" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "issueTokens", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "_owner", "type": "address" } ], "name": "balanceOf", "outputs": [ { "name": "balance", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "symbol", "outputs": [ { "name": "", "type": "string" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_to", "type": "address" }, { "name": "_value", "type": "uint256" } ], "name": "transfer", "outputs": [ { "name": "success", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "_owner", "type": "address" }, { "name": "_spender", "type": "address" } ], "name": "allowance", "outputs": [ { "name": "remaining", "type": "uint256" } ], "payable": false, "type": "function" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "from", "type": "address" }, { "indexed": true, "name": "to", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Transfer", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" }, { "indexed": true, "name": "spender", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Approval", "type": "event" } ]'
eth_swap_contract_abi = '[ { "constant": true, "inputs": [ { "name": "", "type": "uint256" } ], "name": "owners", "outputs": [ { "name": "", "type": "address" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" } ], "name": "removeOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "revokeConfirmation", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "REMCHAIN_ID", "outputs": [ { "name": "", "type": "bytes32" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "address" } ], "name": "isOwner", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "chainId", "type": "bytes32" }, { "name": "swapPubkey", "type": "string" }, { "name": "amountToSwap", "type": "uint256" } ], "name": "requestSwap", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "uint256" }, { "name": "", "type": "address" } ], "name": "confirmations", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "pending", "type": "bool" }, { "name": "executed", "type": "bool" } ], "name": "getTransactionCount", "outputs": [ { "name": "count", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" } ], "name": "addOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "isConfirmed", "outputs": [ { "name": "", "type": "bool" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "getConfirmationCount", "outputs": [ { "name": "count", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "MIN_SWAP_AMOUNT", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "", "type": "uint256" } ], "name": "transactions", "outputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "data", "type": "bytes" }, { "name": "executed", "type": "bool" }, { "name": "swapId", "type": "bytes32" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "getOwners", "outputs": [ { "name": "", "type": "address[]" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "nonce", "type": "uint256" }, { "name": "data", "type": "bytes" } ], "name": "processSwapTransaction", "outputs": [ { "name": "transactionId", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "from", "type": "uint256" }, { "name": "to", "type": "uint256" }, { "name": "pending", "type": "bool" }, { "name": "executed", "type": "bool" } ], "name": "getTransactionIds", "outputs": [ { "name": "_transactionIds", "type": "uint256[]" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "getConfirmations", "outputs": [ { "name": "_confirmations", "type": "address[]" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "transactionCount", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "_required", "type": "uint256" } ], "name": "changeRequirement", "outputs": [], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "REMCHAIN_PUBKEY_LENGTH", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "confirmTransaction", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "destination", "type": "address" }, { "name": "value", "type": "uint256" }, { "name": "data", "type": "bytes" } ], "name": "submitTransaction", "outputs": [ { "name": "transactionId", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "MAX_OWNER_COUNT", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": true, "inputs": [], "name": "required", "outputs": [ { "name": "", "type": "uint256" } ], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "owner", "type": "address" }, { "name": "newOwner", "type": "address" } ], "name": "replaceOwner", "outputs": [], "payable": false, "type": "function" }, { "constant": false, "inputs": [ { "name": "transactionId", "type": "uint256" } ], "name": "executeTransaction", "outputs": [], "payable": false, "type": "function" }, { "inputs": [ { "name": "_owners", "type": "address[]" }, { "name": "_required", "type": "uint256" } ], "payable": false, "type": "constructor" }, { "payable": true, "type": "fallback" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Confirmation", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Revocation", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Submission", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "Execution", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "transactionId", "type": "uint256" } ], "name": "ExecutionFailure", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "sender", "type": "address" }, { "indexed": false, "name": "value", "type": "uint256" } ], "name": "Deposit", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" } ], "name": "OwnerAddition", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": true, "name": "owner", "type": "address" } ], "name": "OwnerRemoval", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "required", "type": "uint256" } ], "name": "RequirementChange", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "sender", "type": "address" }, { "indexed": false, "name": "chainId", "type": "bytes32" }, { "indexed": false, "name": "swapPubkey", "type": "string" }, { "indexed": false, "name": "amountToSwap", "type": "uint256" }, { "indexed": false, "name": "timestamp", "type": "uint256" } ], "name": "SwapRequest", "type": "event" }, { "anonymous": false, "inputs": [ { "indexed": false, "name": "minSwapAmount", "type": "uint256" } ], "name": "MinSwapAmountChange", "type": "event" } ]'

amount_to_swap = 25

eth_address = '0x9f21F19180C8692EBaa061fd231cd1B029Ff2326'
eth_private_key = '3825e93a7999d3c800e62d98016b11b84d9ee10b9849a27cc1905ddf6521f13d'

swap_contract_auth = 'useraaaaaaaa@active'
swap_contract_name = swap_contract_auth.split('@')[0]
contract_pubkey = 'EOS69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D'
producers = [
    'producer111a@active',
    'producer111b@active',
    # 'producer111c@active',
    'useraaaaaaaa@active',
]
txid = '0x83e1b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b8'
remchain_id = '1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32'
swap_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
swap_privkey = '5JGzrjkwagH3MXvMvBdfWPzUrPEwSqrtNCXKdzzbtzjkhcbSjLc'
amount = f'{amount_to_swap}.0000 REM'
block_timestamp = '2019-07-28T20:51:47'
# block_timestamp = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
cleos = 'cleos --url http://127.0.0.1:8001'

sender_auth = 'useraaaaaaab@active'
receiver = 'mynewaccout5'
account_to_create = 'mynewaccout5'
active_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
owner_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'

# account_to_create = ''
# active_pubkey = ''
# owner_pubkey = ''


contract_dir = '~/eosio.contracts/build/contracts'
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
        'gas': 3000000,
        'gasPrice': 100000000000,
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
        'gas': 3000000,
        'gasPrice': 100000000000,
        'nonce': web3.eth.getTransactionCount(eth_address),
        'data': data,
    }
    signed_tx = web3.eth.account.signTransaction(transaction, eth_private_key)
    return web3.eth.sendRawTransaction(signed_tx.rawTransaction).hex()


def step_request_swap():
    web3 = Web3(Web3.WebsocketProvider(eth_provider))

    data = b'0-%.' + encode_abi(['bytes32', 'string', 'uint256'], [bytes.fromhex(remchain_id), swap_pubkey, amount_to_swap*(10**decimals)])
    transaction = {
        'to': eth_swap_contract_address,
        'value': 0,
        'gas': 3000000,
        'gasPrice': 100000000000,
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
    if account_to_create != '':
        swap_id_str = (receiver + "*" + account_to_create + "*" + active_pubkey + "*" + owner_pubkey).encode()
    else:
        swap_id_str = receiver.encode()
    sig = sign(swap_privkey, swap_id_str)
    command = f"""
    {cleos} push action {swap_contract_name} finishswap '[ "{receiver}",
    "{txid}",
    "{remchain_id}",
    "{swap_pubkey}",
    "{amount}", "{block_timestamp}",
    "{sig}",
    "{account_to_create}", "{active_pubkey}",
    "{owner_pubkey}" ]' -p {sender_auth}

    """
    return subprocess.call(command, shell=True)


def check_swap_result():
    command = f"""
    {cleos} get account {receiver}
    """
    subprocess.call(command, shell=True)


if __name__ == '__main__':
    print('issue: ', step_issue_tokens())
    time.sleep(15)
    print('approve: ', step_approve())
    time.sleep(15)
    txid = step_request_swap()
    print('request swap: ', step_request_swap())
    time.sleep(15)
    block_timestamp = get_timestamp(txid)
    #step_set_contract()
    #step_update_auth()
    #step_process_swap()

    finished = False
    while not finished:
        finished = (step_finish_swap() == 0)
        time.sleep(10)
    check_swap_result()
