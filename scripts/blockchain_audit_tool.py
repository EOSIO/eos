#!/usr/bin/env python3

import http.client
import sys
import json

USAGE = "\
Usage: (1) blockchain_audit_tool.py [<RPC ENDPOINT>]\n\
       (2) blockchain_audit_tool.py --help\n\
"

HELP_INFO = "\
    Retrieve info from EOSIO blockchain. If <RPC_ENDPOINT> is not given, it assumed to be 'localhost:8888'\n\
    Outputs to stdout in a human readable format, and write to JSON file blockcahin_audit_data.json:\n\
    Information currently includes:\n\
        Full server info\n\
        All accounts, creation dates, code hashes\n\
        Account permissions\n\
        Producer schedules\n\
        Activated protocol features\n\
        Preview of MI and KV tables on each account\n"


def getJSONResp(conn, rpc, req_body="", exitOnError=True):
    conn.request("POST", rpc, body=req_body)
    resp = conn.getresponse()

    json_resp = resp.read()
    json_data = json.loads(json_resp)
    if 'code' in json_data:
        print(f"ERROR calling '{rpc}'", file=sys.stderr)
        print(f"body= {req_body}", file=sys.stderr)
        print(f"json_resp= {json_resp}", file=sys.stderr)
        if exitOnError:
            exit(1)

    return json_data


if __name__ == "__main__":
    rpc_endpt = "127.0.0.1:8888"
    nArg = len(sys.argv)
    if nArg > 5:
        print(f"ERROR At most four arguments expected, got {nArg}")
        print(USAGE)
        exit(1)
    if nArg > 1:
        arg = sys.argv[1]
        if arg.startswith('-'):
            if arg == '--help':
                print(USAGE)
                print(HELP_INFO)
                exit(0)
            else:
                print(f"ERORR: Unknown option '{arg}'")
                print(USAGE)
                exit(1)

        rpc_endpt = arg

    page_size = 1024
    if len(sys.argv) > 2:
        page_size = int(sys.argv[2])
    scope_limit = 10
    if len(sys.argv) > 3:
        scope_limit = int(sys.argv[3])
    table_row_limit = 10
    if len(sys.argv) > 4:
        table_row_limit = int(sys.argv[4])

    # get the server info
    conn = http.client.HTTPConnection(rpc_endpt)
    server_info_begin = getJSONResp(conn, "/v1/chain/get_info")

    # get all accounts on the chain
    limit = page_size
    moreAccounts = True
    all_accts = []
    req_body = '{"limit":' + str(limit) + '}'
    numAccounts = 0
    print('fetching accounts...', end="", file=sys.stderr)
    print(f'{numAccounts:8}', end="", file=sys.stderr)
    while moreAccounts:
        accts = getJSONResp(conn, "/v1/chain/get_all_accounts", req_body)
        all_accts.append(accts['accounts'])
        numAccounts += len(accts['accounts'])
        print("\b"*8, end="", file=sys.stderr)
        print(f'{numAccounts:8}', end="", file=sys.stderr)
        moreAccounts = 'more' in accts
        if moreAccounts:
            nextAcct = accts['more']
            req_body = '{"limit":' + str(limit) + f', "lower_bound":"{nextAcct}"' + '}'

    if len(all_accts) == 0:
        print("Error, no accounts returned from get_all_accounts!", file=sys.stderr)
        exit(1)

    print('\nfetching account metadata and code hashes... ', end="", file=sys.stderr)
    accts_lst = []
    i = 0
    print(f"############# {i:8}/{numAccounts:8}", end="", file=sys.stderr)
    for accts in all_accts:
        for a in accts:
            i += 1
            nm = a['name']
            print("\b"*31, end="", file=sys.stderr)
            print(f"{nm:13} {i:8}/{numAccounts:8}", end="", file=sys.stderr)

            e = { 'name' : nm }
            req_body = '{ "account_name": "' + nm + '" }'
            metadata = getJSONResp(conn, "/v1/chain/get_account", req_body)

            e['metadata'] = metadata

            # get code hash of account
            code_hash = getJSONResp(conn, "/v1/chain/get_code_hash", req_body)
            e['code_hash'] = code_hash['code_hash']

            accts_lst.append(e)

    print("\nfetching scopes and tables... ", end="", file=sys.stderr)
    print(f"############# {i:8}/{numAccounts:8}", end="", file=sys.stderr)
    i = 0
    for a in accts_lst:
        # get scopes and tables
        nm = a['name']
        print("\b"*31, end="", file=sys.stderr)
        print(f"{nm:13} {i:8}/{numAccounts:8}", end="", file=sys.stderr)
        i += 1

        req_body = '{' + f'"code":"{nm}", "table":"", "lower_bound":"", "upper_bound":"", "limit":{scope_limit}, "reverse":false' + '}'
        scopes = getJSONResp(conn, "/v1/chain/get_table_by_scope", req_body)
        scope_rows = scopes["rows"]
        a['scopes'] = scopes
        a['tables'] = dict()
        for scope in scope_rows:
            sc = scope['scope']
            tbl = scope['table']

            req_body = '{' + f'"json":true, "code":"{nm}", "scope":"{sc}", "table":"{tbl}", "lower_bound":"",\
    "upper_bound":"", "limit": {table_row_limit},"table_key": "",  "key_type": "", "index_position": "",\
    "encode_type": "bytes", "reverse": false, "show_payer": false' + '}'

            table_rows = getJSONResp(conn, "/v1/chain/get_table_rows", req_body, exitOnError=False)
            a['tables'][sc + ":" + tbl] = table_rows

        # get abi so we can determine kv_tables
        req_body = '{' + f'"account_name": "{nm}"' + '}'
        abi = getJSONResp(conn, "/v1/chain/get_abi", req_body)
        if 'abi' in abi:
            kv_tables = abi['abi']['kv_tables']
            for tbl_name, tbl_def in kv_tables.items():
                tbl_idx_nm = tbl_def['primary_index']['name']

                req_body = '{' + f'"json": true,  "code": "{nm}",  "table": "{tbl_name}",\
    "index_name": "{tbl_idx_nm}", "index_value": "", "lower_bound": "", "upper_bound": "",\
    "limit": {table_row_limit},  "encode_type": "bytes",  "reverse": false,  "show_payer": false' + '}'

                table_rows = getJSONResp(conn, "/v1/chain/get_kv_table_rows", req_body, exitOnError=False)
                a['kv_tables'][tbl_name + ":" + tbl_idx_nm] = table_rows

    data = {'accounts' : accts_lst, 'scope_limit' : scope_limit, 'table_row_limit' : table_row_limit }

    prod_sched = getJSONResp(conn, "/v1/chain/get_producer_schedule")
    data['producer_schedule'] = prod_sched

    prot_feats = getJSONResp(conn, "/v1/chain/get_activated_protocol_features")
    prot_feats = prot_feats['activated_protocol_features']
    data['activated_protocol_features'] = prot_feats

    # get deferred transactions
    limit = page_size
    req_body = '{ "json":true, ' + f'"limit":{limit}' + '}'
    trx = getJSONResp(conn, "/v1/chain/get_scheduled_transactions", req_body, exitOnError=False)
    deferred_trx = []
    deferred_trx.extend(trx['transactions'])
    while 'more' in trx and len(trx['more']) > 0:
        deferred_trx.extend(trx['transactions'])
        more_trx = trx["more"]
        req_body = '{"json":true, ' + f'"limit":{limit}, "more":"{more_trx}"' + '}'

        trx = getJSONResp(conn, "/v1/chain/get_scheduled_transactions", req_body, exitOnError=False)

    data['deferred_transactions'] = deferred_trx

    # get the server info again
    server_info_end = getJSONResp(conn, "/v1/chain/get_info")

    data['server_info_begin'] = server_info_begin
    data['server_info_end'] = server_info_end

    # print out results
    try:
        print("                          EOSIO BLOCKCHAIN AUDIT RESULTS\n")
        print(f"             Server Version: {server_info_begin['server_full_version_string']}")
        print(f"                   Chain ID: {server_info_begin['chain_id']}")
        print(f"    Start Head Block / Time: {server_info_begin['head_block_num']} / {server_info_begin['head_block_time']}")
        print(f"      End Head Block / Time: {server_info_end['head_block_num']} / {server_info_end['head_block_time']}")
        print()
        print("     ======  ACCOUNTS CODE ======")
        print("Name            Priv    Created                Last Code Update            Code Hash                     ")
        print("---------------------------------------------------------------------------------------------------------")
        for a in accts_lst:
            m = a['metadata']
            cr = m['created']
            cr = cr[0:21]
            lcu = m['last_code_update']
            lcu = lcu[0:21]
            print(f"{a['name']:13} | {m['privileged']:4} | {cr:21} | {lcu:21} | {a['code_hash']} ")

        print("\n\n     ======  ACCOUNT PERMISSIONS ======")
        print("Accoount        Perm Name       Parent         Auth Threshold / [(Key, Weight)..] / Accounts / Waits                    ")
        print("---------------------------------------------------------------------------------------------------------")
        for a in accts_lst:
            m = a['metadata']
            for p in m['permissions']:
                print(f"{a['name']:13} | {p['perm_name']:13} | {p['parent']:13} | ", end="")
                auth = p['required_auth']
                print(f"{auth['threshold']}", end=" / [")
                for k in auth['keys']:
                    print(f"({k['key']}, {k['weight']})", end=",")
                print("] / [", end="")
                for auth_a in auth['accounts']:
                    print(f"{auth_a}", end=",")
                print("] / [", end="")
                for auth_w in auth['waits']:
                    print(f"{auth_w}", end=",")
                print("]")

        for s in ('active', 'pending', 'proposed'):
            sched = prod_sched[s]
            print(f"\n\n     ====== {s.upper()} PRODUCER SCHEDULE ======")
            if sched is None:
                print("---------------------------------------------------------------------------------------------------------")
                print(" (None)")
                continue
            print(f"Version: {sched['version']}")
            print("Producer Name    Authority  Data Type / Threshold / [ (key, weight), ...] ")
            print("---------------------------------------------------------------------------------------------------------")
            for p in sched['producers']:
                print(f"{p['producer_name']:15} | ", end="")
                auth = p['authority']
                if auth[0] != 0:
                    print(f"ERROR: known block signing authority type expected 0, got {auth[0]}")
                    continue
                a = auth[1]
                keys = a["keys"]
                keys_str = ""
                for k in keys:
                    keys_str += " (" + k['key'] + ", " + str(k['weight']) + "),"

                print(f"{auth[0]} / {a['threshold']} / [{keys_str}] ")

        print("\n\n     ====== ACTIVATED PROTOCOL FEATURES ======")
        print("---------------------------------------------------------------------------------------------------------")
        if len(prot_feats) == 0:
            print(" (None)")
        else:
            for feat in prot_feats:
                print(feat)

        print("\n\n     ====== MULTI-INDEX TABLES ======")
        print("Account        Scope:Table                  Values")
        print("---------------------------------------------------------------------------------------------------------")
        atLeastOne = False
        for a in accts_lst:
            scopes = a['scopes']
            for s in scopes['rows']:
                scope_table = s['scope'] + ":" + s['table']
                print(f"{s['code']:13} | {scope_table:27}", end=" ")
                tbl = a['tables'][scope_table]
                if 'rows' in tbl:
                    print('[', end="")
                    for v in tbl['rows']:
                        print(v, end=", ")
                    if tbl['more']:
                        print("(truncated)", end="")
                    print(']')
                else:
                    print("(no data)")
                atLeastOne = True
            if scopes['more']:
                print(f"{a['name']:13} | (truncated)")
        if not atLeastOne:
            print("(None)")

        print("\n\n     ====== KV TABLES ======")
        print("Account        Table:Primary Index                Values")
        print("---------------------------------------------------------------------------------------------------------")
        atLeastOne = False
        for a in accts_lst:
            if 'kv_tables' not in a:
                continue
            kv_tables = a['kv_tables']
            for tbl_nm_idx, kv_tbl in kv_tables.items():
                print(f"{a['name']:13} | {tbl_nm_idx:27} | [", end="")
                for v in kv_tbl['rows']:
                    print(v, end=", ")
                if kv_tbl['more']:
                    print("(truncated)", end="")
                print(']')
                atLeastOne = True
            if kv_tables['more']:
                print(f"{a['name']:13} | (truncated)")
        if not atLeastOne:
            print("(None)")

        print("\n\n     ====== DEFERRED TRANSACTIONS ======")
        print("---------------------------------------------------------------------------------------------------------")
        if len(deferred_trx) == 0:
            print("(None)")
        else:
            for trx in deferred_trx:
                print(trx)

        print("\nFULL SERVER INFO:\n")
        for k, v in server_info_end.items():
            print(f"{k:>30} : {v}")

    except Exception as e:
        print("\n\n***** exception occured when outputting tables *****")
        print(e)
        raise e

    # save results as json
    with open("blockchain_audit_data.json", "wt") as f:
        f.write(json.dumps(data))
