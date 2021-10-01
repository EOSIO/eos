#!/usr/bin/env python3

import http.client
import sys
import json

def getJSONResp(conn, rpc, req_body=""):
    conn.request("POST", rpc, body=req_body)
    resp = conn.getresponse()
    json_resp = resp.read()
    json_data = json.loads(json_resp)
    if 'code' in json_data:
        print(f"ERROR calling '{rpc}'")
        print(f"body= {req_body}")
        print(f"json_resp= {json_resp}")
        exit(1)

    return json_data

rpc_endpt = "127.0.0.1:8888"
if len(sys.argv) > 1:
    rpc_endpt = sys.argv[1]

page_size = 1024
if len(sys.argv) > 2:
    page_size = int(sys.argv[2])

# get the server info
conn = http.client.HTTPConnection(rpc_endpt)
server_info_begin = getJSONResp(conn, "/v1/chain/get_info")

# get all accounts on the chain
page = 0
isResp = True
all_accts = []
while isResp:
    req_body = '{"page":' + str(page) + ', "page_size":' + str(page_size) + '}'
    accts = getJSONResp(conn, "/v1/chain/get_all_accounts", req_body)
    
    page += 1
    all_accts.append(accts['accounts'])
    isResp = len(accts['accounts']) > 0

if len(all_accts) == 0:
    print("Error, no accounts returned from get_all_accounts!")
    exit(1)

accts_lst = []
for accts in all_accts:
    for a in accts:
        nm = a['name']
        e = { 'name' : nm }

        req_body = '{ "account_name": "' + nm + '" }'
        metadata = getJSONResp(conn, "/v1/chain/get_account", req_body)

        e['metadata'] = metadata

        # get code hash of account
        code_hash = getJSONResp(conn, "/v1/chain/get_code_hash", req_body)
        e['code_hash'] = code_hash['code_hash']

        accts_lst.append(e)

scope_limit = 10
table_row_limit = 10
for a in accts_lst:
    # get scopes and tables
    nm = a['name']
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
        table_rows = getJSONResp(conn, "/v1/chain/get_table_rows", req_body)
        a['tables'][sc + ":" + tbl] = table_rows

    # get abi so we can determine kv_tables
    req_body = '{' + f'"account_name": "{nm}"' + '}'
    abi = getJSONResp(conn, "/v1/chain/get_abi", req_body)
    if 'abi' in abi:
        kv_tables = abi['abi']['kv_tables']
        # TODO: get kv_tables for account

data = {'accounts' : accts_lst}

prod_sched = getJSONResp(conn, "/v1/chain/get_producer_schedule")
data['producer_schedule'] = prod_sched

prot_feats = getJSONResp(conn, "/v1/chain/get_activated_protocol_features")
prot_feats = prot_feats['activated_protocol_features']
data['activated_protocol_features'] = prot_feats

# get the server info again
server_info_end = getJSONResp(conn, "/v1/chain/get_info")

data['server_info_begin'] = server_info_begin
data['server_info_end'] = server_info_end


# print out results
try:
    print("                          EOSIO BLOCKCHAIN AUDIT RESULTS\n")
    print(f"     Server Version: {server_info_begin['server_full_version_string']}")
    print(f"           Chain ID: {server_info_begin['chain_id']}")
    print(f"   Head Block Start: {server_info_begin['head_block_num']}")
    print(f"     Head Block End: {server_info_end['head_block_num']}")
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

    print("\n\n     ====== CONTRACT TABLES (MI) ======")
    print("Account        Scope:Table                  Values")
    print("---------------------------------------------------------------------------------------------------------")
    atLeastOne = False
    for a in accts_lst:
        scopes = a['scopes']
        for s in scopes['rows']:
            scope_table = s['scope'] + ":" + s['table']
            print(f"{s['code']:13} | {scope_table:27}", end=" ")
            tbl = a['tables'][scope_table]
            print('[', end="")
            for v in tbl['rows']:
                print(v, end= ", ")
            if tbl['more']:
                print("(truncated)", end="")
            print(']')
            atLeastOne = True
        if scopes['more']:
            print(f"{a['name']:13} | (truncated)")
    if not atLeastOne:
        print("(None)")       
   
    # TODO: print kv_tables here

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
