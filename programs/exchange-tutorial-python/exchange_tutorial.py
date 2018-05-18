import json
import pprint
import os
import sys
import subprocess
import time

from subprocess import PIPE

# This key would be different for each user.
KEY_TO_INTERNAL_ACCOUNT='12345'
DEMO_USER='scott'

def main():
    try:
        command = sys.argv[1]
        if command == 'monitor':
            setup()
            while True:
                monitor_exchange()
                time.sleep(.1)
        elif command == 'transfer':
            if len(sys.argv) == 4:
                transfer(sys.argv[2], sys.argv[3])
            else:
                print('Transfer must be called by `python exchange_tutorial.py transfer {} 1.0000`'.format(DEMO_USER))
    except subprocess.CalledProcessError as e:
        print(e)
        print(str(e.stderr, 'utf-8'))

def monitor_exchange():
    action_num = get_last_action() + 1
    results = cleos('get actions exchange {} 0 -j'.format(action_num))

    results = json.loads(results.stdout)
    action_list = results['actions']
    if len(action_list) == 0:
        return

    action = action_list[0]
    last_irreversible_block = results['last_irreversible_block']
    to = action['action_trace']['act']['data']['to']
    block_num = action['block_num']

    if is_irreversible(block_num, last_irreversible_block):
        update_balance(action, to)
        set_last_action(action_num)

def update_balance(action, to):
    current_balance = get_balance()
    new_balance = current_balance
    transfer_quantity = action['action_trace']['act']['data']['quantity'].split()[0]
    transfer_quantity = float(transfer_quantity)

    if to == 'exchange':
        if is_valid_deposit(action):
            new_balance = current_balance + transfer_quantity
            set_balance(new_balance)
    elif is_valid_withdrawal(action):
            new_balance = current_balance - transfer_quantity
            set_balance(new_balance)


def transfer(to, quantity):
    if quantity[:-4] != ' EOS':
        quantity += ' EOS'
    results = cleos('transfer exchange {} "{}" {} -j'.format(to, quantity, KEY_TO_INTERNAL_ACCOUNT))
    transaction_info = json.loads(str(results.stdout, 'utf-8'))
    transaction_id = transaction_info['transaction_id']

    transaction_status = transaction_info['processed']['receipt']['status']
    if transaction_status == 'hard_fail':
        print('Transaction failed.')
        return

    add_transactions(transaction_id)
    print('Initiated transfer of {} to {}. Transaction id is {}.'.format(quantity, to, transaction_id))


def is_irreversible(block_num, last_irreversible_block):
    return block_num <= last_irreversible_block

def is_valid_deposit(action):
    account = action['action_trace']['act']['account']
    action_name = action['action_trace']['act']['name']
    memo = action['action_trace']['act']['data']['memo']
    receiver = action['action_trace']['receipt']['receiver']
    token = action['action_trace']['act']['data']['quantity'].split()[1]

    valid_user = action['action_trace']['act']['data']['to'] == 'exchange'
    from_user = action['action_trace']['act']['data']['from']

    # Filter only to actions that notify the exchange account.
    if receiver != 'exchange':
        return False

    if (account == 'eosio.token' and
            action_name == 'transfer' and
            memo == KEY_TO_INTERNAL_ACCOUNT and
            valid_user and
            from_user == DEMO_USER and
            token == 'EOS'):
        return True

    print('Invalid deposit')
    return False

def is_valid_withdrawal(action):
    account = action['action_trace']['act']['account']
    action_name = action['action_trace']['act']['name']
    memo = action['action_trace']['act']['data']['memo']
    receiver = action['action_trace']['receipt']['receiver']
    token = action['action_trace']['act']['data']['quantity'].split()[1]

    transaction_id = action['action_trace']['trx_id']

    valid_user = action['action_trace']['act']['data']['from'] == 'exchange'
    to_user = action['action_trace']['act']['data']['to']

    # Filter only to actions that notify the exchange account.
    if receiver != 'exchange':
        return False

    if (account == 'eosio.token' and
            action_name == 'transfer' and
            memo == KEY_TO_INTERNAL_ACCOUNT and
            valid_user and
            to_user == DEMO_USER and
            transaction_id in get_transactions() and
            token == 'EOS'):
        return True

    print('Invalid withdrawal')
    return False

def cleos(args):
    if isinstance(args, list):
        command = ['./cleos']
        command.extend(args)
    else:
        command = './cleos ' + args

    results = subprocess.run(command, stdin=PIPE, stdout=PIPE, stderr=PIPE, shell=True, check=True)
    return results

def setup():
    if not os.path.exists('last_action.txt'):
        set_last_action(-1)
    if not os.path.exists('balance.txt'):
        set_balance(0)
    if not os.path.exists('transactions.txt'):
        with open('transactions.txt', 'w') as f:
            f.write(json.dumps({"transactions": []}))

def get_transactions():
    with open('transactions.txt', 'r') as f:
        transactions = json.load(f)
        return set(transactions['transactions'])

def add_transactions(transaction_id):
    transactions = get_transactions()
    transactions.add(transaction_id)
    with open('transactions.txt', 'w') as f:
        transactions = json.dumps({'transactions': list(transactions)})
        f.write(transactions)

def get_last_action():
    with open('last_action.txt', 'r') as f:
        last_action = int(f.read())
    return last_action

def set_last_action(action):
    with open('last_action.txt', 'w') as f:
        f.write(str(action))

def get_balance():
    with open('balance.txt', 'r') as f:
        balance = float(f.read())
    return balance

def set_balance(balance):
    with open('balance.txt', 'w') as f:
        f.write(str(balance))
    print("{}'s balance is: {}".format(DEMO_USER, balance))

if __name__ == '__main__':
    main()
