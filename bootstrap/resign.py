from time import sleep

from mainnet_bootstrap import system_accounts, remcode_permission_accounts, remcli, run, jsonArg


def updateAuth(account, permission, parent, controller):

    accounts = [{
            'weight': 1,
            'permission': {'actor': controller, 'permission': 'active'}
        }]
    if account in remcode_permission_accounts and permission == 'active':
        accounts.append({"permission": {"actor": account, "permission": "rem.code"}, "weight": 1})

    run(remcli + 'push action rem updateauth' + jsonArg({
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': {
            'threshold': 1, 'keys': [], 'waits': [],
            'accounts': accounts,
        }
    }) + '-p ' + account + '@' + permission)


def resign(account, controller):
    updateAuth(account, 'owner', '', controller)
    updateAuth(account, 'active', 'owner', controller)
    sleep(1)
    run(remcli + 'get account ' + account)


def resign_system_accounts():
    resign('rem', 'rem.prods')
    for a in system_accounts:
        resign(a, 'rem')


if __name__ == '__main__':
    resign_system_accounts()
