#!/usr/bin/env python3

# By: Jon-Eric Cook
# Github: @joneric

from string import Template
import argparse
import json
import sys
import os
import re

# argument parser
parser = argparse.ArgumentParser(
    prog="abi_to_rc.py",
    description="The abi_to_rc.py script processes a contract's .abi file in order to generate an overview Ricardian Contract and a Ricardian Contract for each action. The overview Ricardian Contract provides a description of the contract's purpose and also specifies the contract's action(s), input(s), and input type(s). The action Ricardian Contract provides a description of the action's purpose and also specifies the action's input(s), and input type(s).",
    epilog="example: $ python abi_to_rc.py ../../contracts/currency/currency.abi",
    usage="$ python %(prog)s [-h] abi_file")
parser.add_argument("abi_file", help="path to smart contract's .abi file")
args = parser.parse_args()

# global variables
_RC_OVERVIEW = "rc-overview-template.md"
_RC_ACTION = "rc-action-template.md"
actions = []
inputs = {}
types = {}

# checks for abi and template files
def check_for_files():
    if not os.path.isfile(args.abi_file):
        abi_filename = os.path.split(args.abi_file)[1]
        print("ERROR: %s could not be found" % abi_filename)
        exit(1)
    if not os.path.isfile(os.path.join(os.path.dirname(sys.argv[0]), _RC_OVERVIEW)):
        print("ERROR: %s could not be found" % _RC_OVERVIEW)
        exit(1)
    if not os.path.isfile(os.path.join(os.path.dirname(sys.argv[0]), _RC_ACTION)):
        print("ERROR: %s could not be found" % _RC_ACTION)
        exit(1)

# gets actions, inputs and input types from abi file
def get_actions_inputs_types():
    abi_file = open(args.abi_file,'r')
    abi_text = abi_file.read()
    abi_file.close()
    abi_json = json.loads(abi_text)
    actions_json = abi_json['actions']
    for obj in actions_json:
        actions.append(obj)
    structs_json = abi_json['structs']
    for action in actions:
        inputs[action['name']] = []
        types[action['name']] = []
    for struct in structs_json:
        for action in actions:
            if struct['name'] == action['type']:
                for field in struct['fields']:
                    inputs[action['name']].append(field['name'])
                    types[action['name']].append(field['type'])

# builds rows for the table
def build_table_rows(is_action):
    table_rows = []
    for action in actions:
        action_string = "`{{ " + action['name'] + " }}`"
        input_string = ""
        input_list = []
        type_string = ""
        type_list =[]
        if len(inputs[action['name']]) >= 1:
            for name in inputs[action['name']]:
                input_list.append("`{{ " + name + ("Var }}`" if is_action else " }}`"))
            input_string = '<br/>'.join(input_list)
        else:
            input_string = "`{{ " + action['type'] + ("Var }}`" if is_action else " }}`")
        if len(types[action['name']]) >= 1:
            for name in types[action['name']]:
                type_list.append("`{{ " + name + " }}`")
            type_string = '<br/>'.join(type_list)
        else:
            type_string = "`{{ " + action['type'] + " }}`"
        table_rows.append('| ' + action_string + ' | ' + input_string + ' | ' + type_string + ' |')
    return table_rows

# generates an overview ricardian contract from the overview template
def generate_rc_overview_file():
    tr = build_table_rows(False)
    abi_file_name = os.path.split(args.abi_file)[1]
    contract_name = os.path.splitext(abi_file_name)[0]
    rc_file_name = contract_name + '-rc.md'
    dirname = os.path.split(args.abi_file)[0]
    subs = {'contract': "{{ " + contract_name + " }}",
            'action': 'actions' if len(actions) > 1 else 'action',
            'input': 'inputs' if len(inputs) > 1 else 'input',
            'type': 'types' if len(types) > 1 else 'type'}
    subs.update([(k+'_header',v.title()) for k,v in subs.copy().items()])
    rc_file = open(os.path.join(dirname, rc_file_name),"w+")
    with open(os.path.join(os.path.dirname(sys.argv[0]), _RC_OVERVIEW)) as fp:
        overview_template = Template(fp.read())
        rc_file.write(overview_template.substitute(subs))
        rc_file.write('\n'.join(tr))
    rc_file.close()

# generates a ricardian contract for each action from the action template
def generate_rc_action_files():
    tr = build_table_rows(True)
    abi_filename = os.path.split(args.abi_file)[1]
    contract_name = os.path.splitext(abi_filename)[0]
    dirname = os.path.split(args.abi_file)[0]
    for action in actions:
        subs = {'action': "{{ " + action['name'] + " }}",
                'input': 'inputs' if len(inputs[action['name']]) > 1 else 'input',
                'type': 'types' if len(types[action['name']]) > 1 else 'type'}
        subs.update([(k+'_header',v.title()) for k,v in subs.copy().items()])
        rc_action_file_name = contract_name + "-" + action['name'] + '-rc.md'
        rc_file = open(os.path.join(dirname, rc_action_file_name),"w+")
        with open(os.path.join(os.path.dirname(sys.argv[0]), _RC_ACTION)) as fp:
            action_template = Template(fp.read())
            rc_file.write(action_template.substitute(subs))
            for row in tr:
                if re.search("\\b" + action['name'] + "\\b", row):
                    rc_file.write(row + "\n")
        rc_file.close()

# main program
def main():
    check_for_files()
    get_actions_inputs_types()
    generate_rc_overview_file()
    generate_rc_action_files()

# runs main
if __name__== "__main__":
    main()