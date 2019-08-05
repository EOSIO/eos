from _sha256 import sha256
from datetime import datetime
from datetime import timezone
from pprint import pprint

from eosiopy import eosio_config
eosio_config.url = 'http://206.189.139.101'
eosio_config.port = 8000
from eosiopy import sign, RawinputParams, EosioParams, NodeNetwork


swap_contract_name = 'remio.swap'
swap_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
swap_privkey = '5JGzrjkwagH3MXvMvBdfWPzUrPEwSqrtNCXKdzzbtzjkhcbSjLc'

tech_privkey = '5JtUScZK2XEp3g9gh7F8bwtPTRAkASmNrrftmx4AxDKD5K4zDnr'
tech_account_auth = 'useraaaaaaaa@active'


def get_swap_id(txid, chain_id, swap_pubkey, amount, timestamp):
    epoch = datetime.utcfromtimestamp(0)
    block_datetime_utc = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S').replace(tzinfo=timezone.utc).timestamp()
    epoch_datetime_utc = datetime.utcfromtimestamp(0).replace(tzinfo=timezone.utc).timestamp()
    seconds_since_epoch = str(int(block_datetime_utc - epoch_datetime_utc))
    print('utc seconds since epoch: ', seconds_since_epoch)
    swap_str = txid + "*" + chain_id + "*" + swap_pubkey[3:] + "*" + amount + "*" + seconds_since_epoch
    print('swap id as str: ', swap_str)
    return sha256(swap_str.encode()).hexdigest()


def get_bound(swap_id):
    return int(swap_id[:16], 16)


def finish_swap(receiver, txid, chain_id, swap_pubkey, asset, block_timestamp, sig, active_pubkey, owner_pubkey):

    raw = RawinputParams("finishswap", {
        "receiver": receiver,
        "txid": txid,
        "chain_id": chain_id,
        "swap_pubkey_str": swap_pubkey,
        "amount": asset,
        "timestamp": block_timestamp,
        "sig": sig,
        "active_pubkey": active_pubkey,
        "owner_pubkey": owner_pubkey,

    }, swap_contract_name, tech_account_auth)
    eosiop_arams = EosioParams(raw.params_actions_list, tech_privkey)
    print('transaction: ', eosiop_arams.trx_json)
    net = NodeNetwork.push_transaction(eosiop_arams.trx_json)
    print('result: ', net)
    """
    on success returns {'transaction_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'processed': {'id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'receipt': {'status': 'executed', 'cpu_usage_us': 3715, 'net_usage_words': 61}, 'elapsed': 3715, 'net_usage': 488, 'scheduled': False, 'action_traces': [{'receipt': {'receiver': 'remio.swap', 'act_digest': 'e85d13825a8f2367ca41f6713a23998024df5ae1e46754c584b59950dd444b2b', 'global_sequence': 1142402, 'recv_sequence': 7, 'auth_sequence': [['useraaaaaaaa', 5]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'remio.swap', 'name': 'finishswap', 'authorization': [{'actor': 'useraaaaaaaa', 'permission': 'active'}], 'data': {'receiver': 'mynewacco1nt', 'txid': '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7', 'chain_id': '1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32', 'swap_pubkey_str': 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC', 'amount': '25.0000 REM', 'timestamp': '2019-07-28T20:51:47.000', 'sig': 'SIG_K1_K2mMta1WZJLWcXNsMRtsPardHspLWmkf4FbLeNAKj3pt53cPToXcyaoD33gTsKJBNU82n4FqeG9GHfDanUb9sMULEXXxA8', 'active_pubkey': 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC', 'owner_pubkey': 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'}, 'hex_data': '9067a00819aea69742307838313631623736336531346432363763323234396331613864313862663262343836616362383038326137396133353865316435643533396439396137376237403163366165373731396132613362346563623139353834613330666635313062613162366465643836653166643862386663323266313137396336323261333235454f53376f4e6d6d786f38796838676d594c55474e43774e4146664c6d724d78746d727a6d46504732394370476d3542713446474390d00300000000000452454d00000000c68ea149001f39676d371cf0fe5a07f9e66aa494bec8b1401db6d1f03b2a795a9b1b910ed7c348f70eea019f3e64fa45c080c9e75e570e33a3d8d65d8d8b1a86d15b207016fa35454f53376f4e6d6d786f38796838676d594c55474e43774e4146664c6d724d78746d727a6d46504732394370476d3542713446474335454f53376f4e6d6d786f38796838676d594c55474e43774e4146664c6d724d78746d727a6d46504732394370476d35427134464743'}, 'context_free': False, 'elapsed': 2001, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': [{'receipt': {'receiver': 'eosio', 'act_digest': '82eac2464b4a895bfffe001eb8466066f6519cb46d5e41ec61abbe09498a9316', 'global_sequence': 1142403, 'recv_sequence': 1141631, 'auth_sequence': [['remio.swap', 7]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio', 'name': 'newaccount', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'creator': 'remio.swap', 'name': 'mynewacco1nt', 'owner': {'threshold': 1, 'keys': [{'key': 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC', 'weight': 1}], 'accounts': [], 'waits': []}, 'active': {'threshold': 1, 'keys': [{'key': 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC', 'weight': 1}], 'accounts': [], 'waits': []}}, 'hex_data': '0040351c03eaa4ba9067a00819aea697010000000100037f7a40478dd5eddaac185aa451f92117e361a28cfd09d24f4660867fd2c4728801000000010000000100037f7a40478dd5eddaac185aa451f92117e361a28cfd09d24f4660867fd2c4728801000000'}, 'context_free': False, 'elapsed': 374, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [{'account': 'mynewacco1nt', 'delta': 2996}], 'except': None, 'inline_traces': []}, {'receipt': {'receiver': 'eosio', 'act_digest': 'f60a9b2feee6fc83b5a9f0a58395b57ce16b615ef3574580b40525eabcd8a5de', 'global_sequence': 1142404, 'recv_sequence': 1141632, 'auth_sequence': [['remio.swap', 8]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio', 'name': 'buyrambytes', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'payer': 'remio.swap', 'receiver': 'mynewacco1nt', 'bytes': 4000}, 'hex_data': '0040351c03eaa4ba9067a00819aea697a00f0000'}, 'context_free': False, 'elapsed': 463, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': [{'receipt': {'receiver': 'eosio.token', 'act_digest': '5d2888e6e61e902351075e18b2acbf6f356015d16084b54f8d825ca39a43a5b9', 'global_sequence': 1142405, 'recv_sequence': 349, 'auth_sequence': [['eosio.ram', 261], ['remio.swap', 9]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}, {'actor': 'eosio.ram', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ram', 'quantity': '0.1159 REM', 'memo': 'buy ram'}, 'hex_data': '0040351c03eaa4ba000090e602ea305587040000000000000452454d00000000076275792072616d'}, 'context_free': False, 'elapsed': 139, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [{'account': 'eosio', 'delta': -128}, {'account': 'remio.swap', 'delta': 128}], 'except': None, 'inline_traces': [{'receipt': {'receiver': 'remio.swap', 'act_digest': '5d2888e6e61e902351075e18b2acbf6f356015d16084b54f8d825ca39a43a5b9', 'global_sequence': 1142406, 'recv_sequence': 8, 'auth_sequence': [['eosio.ram', 262], ['remio.swap', 10]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}, {'actor': 'eosio.ram', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ram', 'quantity': '0.1159 REM', 'memo': 'buy ram'}, 'hex_data': '0040351c03eaa4ba000090e602ea305587040000000000000452454d00000000076275792072616d'}, 'context_free': False, 'elapsed': 66, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}, {'receipt': {'receiver': 'eosio.ram', 'act_digest': '5d2888e6e61e902351075e18b2acbf6f356015d16084b54f8d825ca39a43a5b9', 'global_sequence': 1142407, 'recv_sequence': 87, 'auth_sequence': [['eosio.ram', 263], ['remio.swap', 11]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}, {'actor': 'eosio.ram', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ram', 'quantity': '0.1159 REM', 'memo': 'buy ram'}, 'hex_data': '0040351c03eaa4ba000090e602ea305587040000000000000452454d00000000076275792072616d'}, 'context_free': False, 'elapsed': 5, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}]}, {'receipt': {'receiver': 'eosio.token', 'act_digest': '586b7714aaec1580fcb32dfc52ac6eb0ebbf1152f41a12489904e863527fb979', 'global_sequence': 1142408, 'recv_sequence': 350, 'auth_sequence': [['remio.swap', 12]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ramfee', 'quantity': '0.0006 REM', 'memo': 'ram fee'}, 'hex_data': '0040351c03eaa4baa0d492e602ea305506000000000000000452454d000000000772616d20666565'}, 'context_free': False, 'elapsed': 109, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': [{'receipt': {'receiver': 'remio.swap', 'act_digest': '586b7714aaec1580fcb32dfc52ac6eb0ebbf1152f41a12489904e863527fb979', 'global_sequence': 1142409, 'recv_sequence': 9, 'auth_sequence': [['remio.swap', 13]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ramfee', 'quantity': '0.0006 REM', 'memo': 'ram fee'}, 'hex_data': '0040351c03eaa4baa0d492e602ea305506000000000000000452454d000000000772616d20666565'}, 'context_free': False, 'elapsed': 55, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}, {'receipt': {'receiver': 'eosio.ramfee', 'act_digest': '586b7714aaec1580fcb32dfc52ac6eb0ebbf1152f41a12489904e863527fb979', 'global_sequence': 1142410, 'recv_sequence': 87, 'auth_sequence': [['remio.swap', 14]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'eosio.ramfee', 'quantity': '0.0006 REM', 'memo': 'ram fee'}, 'hex_data': '0040351c03eaa4baa0d492e602ea305506000000000000000452454d000000000772616d20666565'}, 'context_free': False, 'elapsed': 11, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}]}]}, {'receipt': {'receiver': 'eosio.token', 'act_digest': 'b5b47e82323018f5acc0bd5cba243e82e24f6e94f9e04dc6524894d888370996', 'global_sequence': 1142411, 'recv_sequence': 351, 'auth_sequence': [['remio.swap', 15]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'mynewacco1nt', 'quantity': '25.0000 REM', 'memo': '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7'}, 'hex_data': '0040351c03eaa4ba9067a00819aea69790d00300000000000452454d0000000042307838313631623736336531346432363763323234396331613864313862663262343836616362383038326137396133353865316435643533396439396137376237'}, 'context_free': False, 'elapsed': 132, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [{'account': 'remio.swap', 'delta': 240}], 'except': None, 'inline_traces': [{'receipt': {'receiver': 'remio.swap', 'act_digest': 'b5b47e82323018f5acc0bd5cba243e82e24f6e94f9e04dc6524894d888370996', 'global_sequence': 1142412, 'recv_sequence': 10, 'auth_sequence': [['remio.swap', 16]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'mynewacco1nt', 'quantity': '25.0000 REM', 'memo': '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7'}, 'hex_data': '0040351c03eaa4ba9067a00819aea69790d00300000000000452454d0000000042307838313631623736336531346432363763323234396331613864313862663262343836616362383038326137396133353865316435643533396439396137376237'}, 'context_free': False, 'elapsed': 63, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}, {'receipt': {'receiver': 'mynewacco1nt', 'act_digest': 'b5b47e82323018f5acc0bd5cba243e82e24f6e94f9e04dc6524894d888370996', 'global_sequence': 1142413, 'recv_sequence': 1, 'auth_sequence': [['remio.swap', 17]], 'code_sequence': 1, 'abi_sequence': 1}, 'act': {'account': 'eosio.token', 'name': 'transfer', 'authorization': [{'actor': 'remio.swap', 'permission': 'active'}], 'data': {'from': 'remio.swap', 'to': 'mynewacco1nt', 'quantity': '25.0000 REM', 'memo': '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7'}, 'hex_data': '0040351c03eaa4ba9067a00819aea69790d00300000000000452454d0000000042307838313631623736336531346432363763323234396331613864313862663262343836616362383038326137396133353865316435643533396439396137376237'}, 'context_free': False, 'elapsed': 5, 'console': '', 'trx_id': '8fb305cd4938652408a6a18246d01da548befb7310a56a810f3238bee92810c4', 'block_num': 1140841, 'block_time': '2019-08-01T11:14:18.000', 'producer_block_id': None, 'account_ram_deltas': [], 'except': None, 'inline_traces': []}]}]}], 'except': None}}
    if transaction executes  but fails {'code': 500, 'message': 'Internal Service Error', 'error': {'code': 3050003, 'name': 'eosio_assert_message_exception', 'what': 'eosio_assert_message assertion failure', 'details': [{'message': 'assertion failure with message: swap already exists', 'file': 'wasm_interface.cpp', 'line_number': 924, 'method': 'eosio_assert'}]}}
    if transaction can't be broadcast {'code': 500, 'message': 'Internal Service Error', 'error': {'code': 3090003, 'name': 'unsatisfied_authorization', 'what': 'Provided keys, permissions, and delays do not satisfy declared authorizations', 'details': [{'message': "transaction declares authority '${auth}', but does not have signatures for it under a provided delay of 0 ms, provided permissions ${provided_permissions}, provided keys ${provided_keys}, and a delay max limit of 3888000000 ms", 'file': 'authorization_manager.cpp', 'line_number': 520, 'method': 'check_authorization'}]}}
    """


def sign_digest(receiver, active_pubkey, owner_pubkey):
    if active_pubkey != '' and owner_pubkey != '':
        digest_to_sign = (receiver + "*" + active_pubkey[3:] + "*" + owner_pubkey[3:]).encode()
    else:
        digest_to_sign = receiver.encode()
    print('digest to sign: ', digest_to_sign)
    sig = sign(swap_privkey, digest_to_sign)
    print('signature: ', sig)

    return sig


if __name__ == '__main__':
    txid = '0x8161b763e14d267c2249c1a8d18bf2b486acb8082a79a358e1d5d539d99a77b7'
    chain_id = '1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32'
    amount_to_swap = 25
    block_timestamp = '2019-07-28T20:51:47'
    asset = f'{amount_to_swap}.0000 REM'

    swap_id = get_swap_id(txid, chain_id, swap_pubkey, asset, '2019-07-28T20:51:47')
    print('swap_id: ', swap_id)
    print('bound: ', get_bound(swap_id))

    receiver = 'mynewacco1nt'
    active_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
    owner_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
    sig = sign_digest(receiver, active_pubkey, owner_pubkey)

    finish_swap(receiver, txid, chain_id, swap_pubkey, asset, block_timestamp, sig, active_pubkey, owner_pubkey)
