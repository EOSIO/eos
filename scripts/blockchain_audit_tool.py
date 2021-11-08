#!/usr/bin/env python3

import http.client
import sys
import json
import re

USAGE = "\
Usage: blockchain_audit_tool.py [OPTIONS] [<rpc_endpoint>]\n"

HELP_INFO = "\
    Audit EOSIO blockchain.\n\
    <rpc_endpoint> RPC endpoint URI of nodeos with chain_api_plugin enabled. Defaults to 'localhost:8888'\n\
    OPTIONS:\n\
                  --help  Print help info and exit\n\
       --comp <filepath>  Do not query nodeos.  Instead use <filepath> as basis for comparison combined with '--ref' option\n\
           -o <filepath>  Output to filepath instead of default 'blockchain_audit_data.json'\n\
       --page_size <num>  Integer > 0, default 1024. The 'limit' field in RPC queries for accounts and table data.  Default 1024\n\
        --ref <filepath>  Use <filepath> to comparefor non-transient data to use comparison\n\
     --scope-limit <num>  Integer >= 0, default 0. Max # of scopes to fetch. 0=no limit\n\
 --table-row-limit <num>  Integer >= 0, default 0. Max # of rows of MI and KV tables to fetch. 0=no limit\n\
       --keep-irrelevant  Flag.  Normally time and block number data will be removed from the JSON.  Setting this flag keeps that data.\n\
    Outputs to stdout in a human readable format, and write to JSON file specified by '-o' which defaults to:\n\
         'blockchain_audit_data.json'\n\
    Status and error info is written to stderr.\n\
    Information currently includes:\n\
        Server info\n\
        All accounts, creation dates, code hashes\n\
        Account permissions\n\
        Producer schedules\n\
        Activated protocol features\n\
        Preview of MI and KV tables on each account\n\
        Deferred transactions"

SERVER_INFO_TRANSIENT_FIELDS = [ "head_block_num", "last_irreversible_block_num",
    "last_irreversible_block_id", "head_block_id",  "head_block_time",
    "fork_db_head_block_num", "fork_db_head_block_id", "server_full_version_string",
    "last_irreversible_block_time", "virtual_block_cpu_limit", "virtual_block_net_limit" ]

class ArgumentParseError(Exception):
    def __init__(self, msg):
        Exception()
        self.msg = msg

# parse argument option
# assumes argv[i] starts with '-'
# options map is an map of the argument to a 2-tuple of the option name
# and default parameter value
# if default value is not None, it will attempt to parse the parameter
# either with an '-option=value' ro an '-option value' semantic
# if default value is None, the param value will be 'True' (bool type)
# to indicate parameter is present
# otherwise parameter values will remain strings (no type conversion is attempted)
# returns nexted tuple ((option, value), i)
def parseArgOption(argv, i, optionsMap):
    arg = argv[i]

    nArg = len(argv)
    if arg in optionsMap:
        e = optionsMap[arg]
        option = e[0]
        if e[1] is None:  # no parameter, assume boolean
            return (option, True), i+1
        elif isinstance(e[1], tuple):
            reqArgs = len(e[1])
            i += 1
            if i + reqArgs > nArg:
                raise ArgumentParseError(f"option '{arg}' expected {reqArgs} parameters")
            return (option, tuple(argv[i:i+reqArgs])), i+reqArgs
        else:
            i += 1
            if i >= nArg:
                raise ArgumentParseError(f"option '{arg}' expected parameter")
            a = argv[i]
            # for integer arguments, attempt conversion
            if isinstance(e[1], int):
                try:
                    a = int(a)
                except:
                    raise ArgumentParseError(f"option '{arg}' expected integer argument, got '{arg[i]}'")
            return (option, a), i+1
    else:
        r = re.split("=", arg)
        if r[0] not in optionsMap:
            raise ArgumentParseError(f"unkonwn option '{r[0]}'")
        if len(r) > 2:
            raise ArgumentParseError(f"option '{arg}' multiple equal signs")
        e = optionsMap[r[0]]
        a = r[1]
        # for integer arguments, attempt conversion
        if isinstance(e[1], int):
            try:
                a = int(a)
            except:
                raise ArgumentParseError(f"option '{r[0]}' expected integer argument, got '{r[1]}'")
        return (e[0], a), i+1


# parse arguments given command line argv, map of option arguments to option names and default values
# min/max other arguments
# returns 2-tuple, (optionMapOut, otherArgsList)
#   optionMapOut is a map of the option name to the value
#   otherArgsList is list of non-pption arguments
# throw ArgumentParseError if parse fails for any reason
def parseArgs(argv, optionsMapIn, otherArgsMin, otherArgsMax):
    nArg = len(argv)
    i = 1
    optionsMapOut = dict()
    # populate options with default values
    for k, v in optionsMapIn.items():
        optionsMapOut[v[0]] = v[1]
        if v[1] is None:
            optionsMapOut[v[0]] = False

    otherArgsList = []
    while i < nArg:
        arg = argv[i]
        if arg.startswith("-"):
            option, i = parseArgOption(argv, i, optionsMapIn)
            optionsMapOut[option[0]] = option[1]
        else:
            otherArgsList.append(arg)
            i += 1

    nOtherArgs = len(otherArgsList)
    if nOtherArgs > otherArgsMax:
        raise ArgumentParseError(f"too many non-option arguments supplied, got {nOtherArgs}, max is {otherArgsMax}")
    if nOtherArgs < otherArgsMin:
        raise ArgumentParseError(f"not enough non-option arguments supplied, got {nOtherArgs}, min is {otherArgsMin}")

    return optionsMapOut, otherArgsList


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


def getAllAccounts(conn, limit):
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
    return all_accts, numAccounts

def getScopes(conn, code_name, page_size, max_scopes):
    next_scope = ""
    i = 0
    scope_rows_all = []
    moreData = True
    while moreData and (max_scopes == 0 or i < max_scopes):
        req_body = '{' + f'"code":"{code_name}", "table":"", "lower_bound":"{next_scope}", "upper_bound":"", "limit":{page_size}, "reverse":false' + '}'
        scopes = getJSONResp(conn, "/v1/chain/get_table_by_scope", req_body)
        scope_rows_all.extend(scopes["rows"])
        next_scope = scopes["more"]
        i += page_size
        moreData = (next_scope != "")

    return scope_rows_all

def getTableRows(conn, code_name, scope_name, table_name, page_size, max_rows):
    next_table = ""
    i = 0
    table_rows_all = []
    moreData = True
    while moreData and (i < max_rows or max_rows == 0):
        req_body = '{' + f'"json":true, "code":"{code_name}", "scope":"{scope_name}", "table":"{table_name}", "lower_bound":"{next_table}",\
    "upper_bound":"", "limit": {page_size},"table_key": "",  "key_type": "", "index_position": "",\
    "encode_type": "bytes", "reverse": false, "show_payer": false' + '}'

        resp = getJSONResp(conn, "/v1/chain/get_table_rows", req_body, exitOnError=False)
        try:
            table_rows_all.extend(resp["rows"])

            next_table = resp["more"]
            moreData = (next_table != "")
            i += page_size
        except Exception as e:
            break

    return table_rows_all

def getKVTableData(conn, code_name, page_size, max_rows):
    next_key = ""
    i = 0
    kv_data_all = []
    moreData = True
    while moreData and (i < max_rows or max_rows == 0):
        req_body = '{' + f'"json": false,  "code": "{code_name}",  "table": "{next_key}",\
"index_name": "", "index_value": "", "lower_bound": "", "upper_bound": "",\
"limit": {page_size},  "encode_type": "bytes",  "reverse": false,  "show_payer": false' + '}'

        # print(f"getKVTableData: req_body={req_body}")
        resp = getJSONResp(conn, "/v1/chain/get_kv_table_rows", req_body, exitOnError=False)
        try:
            kv_data_all.extend(resp["rows"])

            moreData = resp['more']
            next_key = resp["next_key"]
            i += page_size
        except Exception as e:
            break

    return kv_data_all

def compareAccountNames(ref, cmp):
    mismatch = []
    for nm in ref:
        if nm not in cmp:
            mismatch.append((nm, '(none)'))
    for nm in cmp:
        if nm not in ref:
            mismatch.append(('(none)', nm))
    return mismatch


def compareAccountsField(ref, cmp, fieldName, subFieldName=None):
    mismatches = []
    for nm, ref_acct in ref.items():
        if nm in cmp:
            ref_v = ref_acct[fieldName]
            if subFieldName is not None:
                ref_v = ref_v[subFieldName]
            cmp_v = cmp[nm][fieldName]
            if subFieldName is not None:
                cmp_v = cmp_v[subFieldName]
            if ref_v != cmp_v:
                mismatches.append((nm, ref_v, cmp_v))
    return mismatches


def compareServerInfo(ref, cmp):
    fields = ('server_version', 'chain_id', 'head_block_producer', 'virtual_block_cpu_limit',
              'virtual_block_net_limit', 'block_cpu_limit', 'block_net_limit', 'server_version_string',
              'server_full_version_string')
    mismatches = []
    for f in fields:
        if f in ref and f not in cmp:
            mismatches.append((f, ref[f], None))
        elif f in cmp and f not in ref:
            mismatches.append((f, None, cmp[f]))
        elif f not in ref and f not in cmp:
            continue
        elif ref[f] != cmp[f]:
            mismatches.append((f, ref[f], cmp[f]))
    return mismatches


def compareProtocolFeatures(ref, cmp):
    fieldNames = ('feature_digest', 'activation_ordinal', 'description_digest', 'dependencies',
                  'protocol_feature_type', 'specification')
    mismatches = []
    for digest, ref_feat in ref.items():
        if digest in cmp:
            cmp_feat = cmp[digest]
            for f in fieldNames:
                if ref_feat[f] != cmp_feat[f]:
                    mismatches.append((f"digest '{digest}' field '{f}'", ref_feat[f], cmp_feat[f]))
        else:
            mismatches.append(('digest', digest, '(none)'))
    return mismatches


def compareDeferredTrx(ref, cmp):
    mismatches = []
    if ref != cmp:
        mismatches.append("transactions", ref, cmp)
    return mismatches


def compareProduceSchedules(ref, cmp):
    mismatches = []
    for s in ('active', 'pending', 'proposed'):
        if ref[s] != cmp[s]:
            mismatches.append((s, ref[s], cmp[s]))
    return mismatches


def compareRefData(refData, cmpData):
    # build dictionary of account names to ease in comparison
    refAccts = dict()
    for a in refData['accounts']:
        refAccts[a['name']] = a
    cmpAccts = dict()
    for a in cmpData['accounts']:
        cmpAccts[a['name']] = a

    # build dictionary of feature digest to protocol feature
    refProtFeats = dict()
    for feat in refData['activated_protocol_features']:
        refProtFeats[feat['feature_digest']] = feat
    cmpProtFeats = dict()
    for feat in cmpData['activated_protocol_features']:
        cmpProtFeats[feat['feature_digest']] = feat

    mismatches = dict()
    mismatches['account_names'] = compareAccountNames(refAccts, cmpAccts)
    mismatches['accounts_privilege'] = compareAccountsField(refAccts, cmpAccts, 'metadata', 'privileged')
    mismatches['accounts_hash'] = compareAccountsField(refAccts, cmpAccts, 'code_hash')
    mismatches['accounts_scopes'] = compareAccountsField(refAccts, cmpAccts, 'scopes')
    mismatches['accounts_tables'] = compareAccountsField(refAccts, cmpAccts, 'tables')
    mismatches['accounts_kv_tables'] = compareAccountsField(refAccts, cmpAccts, 'kv_tables')
    mismatches['accounts_permissions'] = compareAccountsField(refAccts, cmpAccts, 'metadata', 'permissions')
    mismatches['producer_schedule'] = compareProduceSchedules(refData['producer_schedule'], cmpData['producer_schedule'])
    mismatches['deferred_trx'] = compareDeferredTrx(refData['deferred_transactions'], cmpData['deferred_transactions'])
    mismatches['server_info'] = compareServerInfo(refData['server_info_begin'], cmpData['server_info_begin'])
    mismatches['protocol_features'] = compareProtocolFeatures(refProtFeats, cmpProtFeats)

    isSame = True
    for k, v in mismatches.items():
        if len(v) > 0:
            isSame = False
            print(f"ERROR: mismatch in '{k}'", file=sys.stderr)
            for i in v:
                print(f"  '{i[0]}' ref: {i[1]}  comp: {i[2]}", file=sys.stderr)
    if isSame:
        print("Reference comparison SUCCESS", file=sys.stderr)
    else:
        print("Reference comparison FAILURE - see errors above.", file=sys.stderr)
    return isSame

def removeIrrelevant(data):
    inf_begin = data["server_info_begin"]
    inf_end = data["server_info_end"]
    for fld in SERVER_INFO_TRANSIENT_FIELDS:
        del inf_begin[fld]
        del inf_end[fld]
    accts = data["accounts"]
    for a in accts:
        md = a["metadata"]
        del md["head_block_num"]
        del md["head_block_time"]
        del md["last_code_update"]
        del md["created"]
        del md["net_limit"]["last_usage_update_time"]
        del md["cpu_limit"]["last_usage_update_time"]

    for feat in data["activated_protocol_features"]:
        del feat["activation_block_num"]


if __name__ == "__main__":
    rpc_endpt = "127.0.0.1:8888"
    optionsMap = {
               "--comp" : ("comp-filepath", ""),
               "--help" : ("help", None),
                   "-o" : ("output-filepath", "blockchain_audit_data.json"),
          "--page-size" : ("page-size", 1024),
                "--ref" : ("reference-filepath", ""),
        "--scope-limit" : ("scope-limit", 0),
    "--table-row-limit" : ("table-row-limit", 0),
    "--keep-irrelevant" : ("keep-irrelevant", None)
                  }

    # parse options
    optionsMap, otherArgLst = parseArgs(sys.argv, optionsMap, 0, 1)
    if optionsMap['help']:
        print(USAGE, file=sys.stderr)
        print(HELP_INFO, file=sys.stderr)
        exit(0)

    if len(otherArgLst) > 0:
        rpc_endpt = otherArgLst[0]

    page_size = optionsMap['page-size']
    scope_limit = optionsMap['scope-limit']
    table_row_limit = optionsMap['table-row-limit']
    output_filepath = optionsMap['output-filepath']
    ref_filepath = optionsMap['reference-filepath']
    comp_filepath = optionsMap['comp-filepath']
    keep_irrelevant = optionsMap['keep-irrelevant']

    if scope_limit < 0:
        print("scope-limit must be >= 0")
        print(USAGE)
        exit(1)
    if table_row_limit < 0:
        print("scope-limit must be >= 0")
        print(USAGE)

    if comp_filepath != "":
        if ref_filepath == "":
            print("'--comp 'option must be used in combination with '--ref' option", file=sys.stderr)
            exit(1)
        fComp = open(comp_filepath, "r")
        s = fComp.read()
        cmpData = json.loads(s)
        fRef = open(ref_filepath, "r")
        s = fRef.read()
        refData = json.loads(s)
        compSuccess = compareRefData(refData, cmpData)
        if compSuccess:
            exit(0)
        exit(1)

    # establish connection to nodeos
    conn = http.client.HTTPConnection(rpc_endpt)

    # get the server info
    server_info_begin = getJSONResp(conn, "/v1/chain/get_info")

    # get all accounts on the chain
    all_accts, numAccounts = getAllAccounts(conn, page_size)
    if numAccounts == 0:
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
            print(f"{nm:13} {i:8}/{numAccounts:8}", end="", file=sys.stderr, flush=True)

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
        print(f"{nm:13} {i:8}/{numAccounts:8}", end="", file=sys.stderr, flush=True)
        i += 1

        scopes = getScopes(conn, nm, page_size, scope_limit)

        a['scopes'] = scopes
        a['tables'] = dict()
        for scope in scopes:
            sc = scope['scope']
            tbl = scope['table']

            table_rows = getTableRows(conn, nm, sc, tbl, page_size, table_row_limit)

            a['tables'][sc + ":" + tbl] = table_rows

        kv_data = getKVTableData(conn, nm, page_size, table_row_limit)
        a['kv_tables'] = kv_data

    data = {'accounts' : accts_lst, 'scope_limit' : scope_limit, 'table_row_limit' : table_row_limit }
    print(file=sys.stderr)

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

    # get all accounts again
    all_accts_end, numAccounts_end = getAllAccounts(conn, page_size)
    print(file=sys.stderr)
    if numAccounts != numAccounts_end:
        msg = f"ERROR: Accounts added during audit. begin= {numAccounts} end= {numAccounts_end}"
        print(msg, file=sys.stderr)

    if server_info_begin['head_block_num'] != server_info_end['head_block_num']:
        print("WARNING: Audit data was collected from different blocks.  Data may be inconssitent.", file=sys.stderr)

    # print out results
    try:
        print("                          EOSIO BLOCKCHAIN AUDIT RESULTS\n")
        print(f"             Server Version: {server_info_begin['server_full_version_string']}")
        print(f"                   Chain ID: {server_info_begin['chain_id']}")
        print(f"    Start Head Block / Time: {server_info_begin['head_block_num']} / {server_info_begin['head_block_time']}")
        print(f"      End Head Block / Time: {server_info_end['head_block_num']} / {server_info_end['head_block_time']}")
        print()
        print("     ======  ACCOUNTS CODE ======")
        print("Name            Privilege    Created                Last Code Update            Code Hash                     ")
        print("---------------------------------------------------------------------------------------------------------")
        for a in accts_lst:
            m = a['metadata']
            cr = m['created']
            cr = cr[0:21]
            lcu = m['last_code_update']
            lcu = lcu[0:21]
            print(f"{a['name']:13} | {m['privileged']:9} | {cr:21} | {lcu:21} | {a['code_hash']} ")

        print("\n\n     ======  ACCOUNT PERMISSIONS AND PERMISSION LINKS ======")
        print("Accoount        Perm Name       Parent         Auth Threshold / [(Key, Weight)..] / Accounts / Waits               ")
        print("-------------------------------------------------------------------------------------------------------------------")
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

                print("     Linked Actions: ", end="")
                if 'linked_actions' in p:
                    linked_acts = p['linked_actions']
                    if len(linked_acts) == 0:
                        print("(None)", end="")
                    for l_act in linked_acts:
                        print(f'{l_act["account"]}::{l_act["action"]}, ', end="")
                    print()
                else:
                    print('(Unkonwn)')

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
            for s in scopes:
                scope_table = s['scope'] + ":" + s['table']
                print(f"{s['code']:13} | {scope_table:27}", end=" ")
                tbl = a['tables'][scope_table]
                print('[', end="")
                for v in tbl:
                    print(v, end=", ")
                print(']')
                atLeastOne = True
        if not atLeastOne:
            print("(None)")

        print("\n\n     ====== KV TABLES ======")
        print("Account        Data")
        print("---------------------------------------------------------------------------------------------------------")
        for a in accts_lst:
            print(f"{a['name']:13}", end="")
            kv_tables = a['kv_tables']
            print("[", end="")
            for v in kv_tables:
                print(v, end=", ")

            print(']')

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
    with open(output_filepath, "wt") as f:
        if not keep_irrelevant:
            removeIrrelevant(data)

        f.write(json.dumps(data, sort_keys=True, indent=2))

    if ref_filepath != "":
        with open(ref_filepath, "r") as f:
            s = f.read()
            refData = json.loads(s)
            compSuccess = compareRefData(refData, data)
            if compSuccess:
                exit(0)
            exit(1)
