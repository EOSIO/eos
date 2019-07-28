import subprocess
from datetime import datetime
from eosiopy import sign

swap_contract_auth = 'useraaaaaaaa@active'
swap_contract_name = swap_contract_auth.split('@')[0]
contract_pubkey = 'EOS69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D'
producers = [
    'producer111a@active',
    'producer111b@active',
    # 'producer111c@active',
    'useraaaaaaaa@active',
]
txid = 'b194c53ac3df12704faf4247059d903ef5e9bd4f80dc393fba248114a86a7dd4'
chain_id = '0000000000000000000000000000000000000000000000000000000000000000'
swap_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
swap_privkey = '5JGzrjkwagH3MXvMvBdfWPzUrPEwSqrtNCXKdzzbtzjkhcbSjLc'
amount = '2.0000 REM'
block_timestamp = '2019-07-28T03:38:39'
# block_timestamp = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
print(block_timestamp)
cleos = 'cleos --url http://127.0.0.1:8001'

sender_auth = 'useraaaaaaab@active'
receiver = 'mynewaccout3'
account_to_create = 'mynewaccout3'
active_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'
owner_pubkey = 'EOS7oNmmxo8yh8gmYLUGNCwNAFfLmrMxtmrzmFPG29CpGm5Bq4FGC'

# account_to_create = ''
# active_pubkey = ''
# owner_pubkey = ''


contract_dir = '~/eosio.contracts/build/contracts'
swap_contract_path = '~/eos/tutorials/bios-boot-tutorial/remio.swap'


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
        "{chain_id}",
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
    "{chain_id}",
    "{swap_pubkey}",
    "{amount}", "{block_timestamp}",
    "{sig}",
    "{account_to_create}", "{active_pubkey}",
    "{owner_pubkey}" ]' -p {sender_auth}

    """
    subprocess.call(command, shell=True)


def check_swap_result():
    command = f"""
    {cleos} get account {receiver}
    """
    subprocess.call(command, shell=True)


if __name__ == '__main__':
    step_set_contract()
    step_update_auth()
    step_process_swap()
    step_finish_swap()
    check_swap_result()
