#!/usr/bin/env python3

import http.client
import sys
import json

rpc_endpt = "127.0.0.1:8888"
if len(sys.argv) > 1:
    rpc_endpt = sys.argv[1]

page_size = 1024
if len(sys.argv) > 2:
    page_size = int(sys.argv[2])

# get the server info
conn = http.client.HTTPConnection(rpc_endpt)
conn.request("POST", "/v1/chain/get_info")
resp = conn.getresponse()
info_json = resp.read()
server_info_begin = json.loads(info_json)

# get all accounts on the chain
page = 0
isResp = True
all_accts = []
while isResp:
    req_body = '{"page":' + str(page) + ', "page_size":' + str(page_size) + '}'
    conn.request("POST", "/v1/chain/get_all_accounts", body=req_body)

    resp = conn.getresponse()
    accts_json = resp.read()
    accts = json.loads(accts_json)
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
        conn.request("POST", "/v1/chain/get_account", body=req_body)
        resp = conn.getresponse()
        acct_resp = resp.read()
        metadata = json.loads(acct_resp)
        e['privileged'] = metadata['privileged']
        e['permissions'] = metadata['permissions']

        # get code hash of account
        conn.request("POST", "/v1/chain/get_code_hash", body=req_body)
        resp = conn.getresponse()
        code_resp = resp.read()
        code_hash = json.loads(code_resp)
        e['code_hash'] = code_hash['code_hash']

        accts_lst.append(e)

data = {'accounts' : accts_lst}

conn.request("POST", "/v1/chain/get_producer_schedule")
resp = conn.getresponse()
prod_sched = json.loads(resp.read())
data['producer_schedule'] = prod_sched

conn.request("POST", "/v1/chain/get_activated_protocol_features")
resp = conn.getresponse()
prot_feats = json.loads(resp.read())
prot_feats = prot_feats['activated_protocol_features']
data['activated_protocol_features'] = prot_feats

# get the server info again
conn.request("POST", "/v1/chain/get_info")
resp = conn.getresponse()
info_json = resp.read()
server_info_end = json.loads(info_json)

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
    print("Name            Priv    Code Hash")
    print("---------------------------------------------------------------------------------------------------------")
    for a in accts_lst:
        print(f"{a['name']:13} | {a['privileged']:5} | {a['code_hash']}")

    print("\n\n     ======  ACCOUNT PERMISSIONS ======")
    print("Accoount        Perm Name       Parent         Auth Threshold / Keys / Accounts / Weights                    ")
    print("---------------------------------------------------------------------------------------------------------")
    for a in accts_lst:
        for p in a['permissions']:
            print(f"{a['name']:13} | {p['perm_name']:13} | {p['parent']:13} | ", end="")
            auth = p['required_auth']
            print(f"{auth['threshold']}", end=" / ")
            if len(auth['keys']) == 0:
                print("(none)", end="")
            for k in auth['keys']:
                print(f"({k['key']}, w={k['weight']})", end=",")
            print(" / ", end="")
            if len(auth['accounts']) == 0:
                print("(none)", end="")
            for auth_a in auth['accounts']:
                print(f"{auth_a}", end=",")
            print(" / ", end="")
            if len(auth['waits']) == 0:
                print("(none)", end="")
            for auth_w in auth['waits']:
                print(f"{auth_w}", end=",")
            print()

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
except Exception as e:
    print("\n\n***** exception occured when outputting tables *****")
    print(e)

# save results as json
with open("blockchain_audit_data.json", "wt") as f:
    f.write(json.dumps(data))
