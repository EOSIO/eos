#!/usr/bin/env python3

# By: Jon-Eric Cook
# Github: @joneric

import sys
import os
import json

# global variables
actions = []
inputs = {}
types = {}

# validates passed in arguments
def validate_input_arguments():
    # exit if less than 4 arguments are given
    if len(sys.argv) < 4:
        print("ERROR: missing arguments")
        print("-- Expected format --")
        print("$ python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md")
        sys.exit(0)

    # exit if arguments are ordered incorrectly
    # exit if passed in files can't be found
    elif len(sys.argv) == 4:
        if ".abi" not in sys.argv[1]:
            print("ERROR: first argument should be a .abi file")
            print("-- Expected format --")
            print("$ python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md")
            sys.exit(0)
        elif not os.path.isfile(sys.argv[1]):
            print("ERROR: %s could not be found" % sys.argv[1])
            sys.exit(0)
        elif "rc-overview-template.md" != sys.argv[2]:
            print("ERROR: second argument should be the 'rc-overview-template.md' file")
            print("-- Expected format --")
            print("$ python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md")
            sys.exit(0)
        elif not os.path.isfile(sys.argv[2]):
            print("ERROR: %s could not be found" % sys.argv[2])
            sys.exit(0)
        elif "rc-action-template.md" != sys.argv[3]:
            print("ERROR: third argument should be the 'rc-action-template.md' file")
            print("-- Expected format --")
            print("$ python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md")
            sys.exit(0)
        elif not os.path.isfile(sys.argv[3]):
            print("ERROR: %s could not be found" % sys.argv[3])
            sys.exit(0)
    else:
        print("ERROR: too many arguments")
        print("-- Expected format --")
        print("$ python abi_to_rc.py ${{smart-contract.abi}} rc-overview-template.md rc-action-template.md")
        sys.exit(0)

# gets actions, inputs and input types from abi file
def get_actions_inputs_types():
    abi_file_name = sys.argv[1]
    abi_file = open(abi_file_name,'r')
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
def build_table_rows():
    table_rows = []
    row = "| `${{ action }}` | `${{ input }}` | `${{ type }}` |"
    for action in actions:
        temp_row = row
        temp_row = temp_row.replace("${{ action }}",action['name'])
        if len(inputs[action['name']]) > 1:
            input_string = ""
            for name in inputs[action['name']]:
                input_string = input_string + "`" + name + "`" + "<br/>"
            temp_row = temp_row.replace("`${{ input }}`",input_string)
        elif len(inputs[action['name']]) == 1:
            temp_row = temp_row.replace("${{ input }}",inputs[action['name']][0])
        else:
            temp_row = temp_row.replace("${{ input }}",action['type'])

        if len(types[action['name']]) > 1:
            type_string = ""
            for name in types[action['name']]:
                type_string = type_string + "`" + name + "`" + "<br/>"
            temp_row = temp_row.replace("`${{ type }}`",type_string)
        elif len(types[action['name']]) == 1:
            temp_row = temp_row.replace("${{ type }}",types[action['name']][0])
        else:
            temp_row = temp_row.replace("${{ type }}",action['type'])

        table_rows.append(temp_row)
    return table_rows

# generates an overview ricardian contract from the overview template
def generate_rc_overview_file():
    tr = build_table_rows()
    rc_overview_template_file_name = sys.argv[2]
    abi_file_name = sys.argv[1]
    contract_name = abi_file_name[:-4]
    rc_file_name = contract_name + '-rc.md'
    rc_file = open(rc_file_name,"w+")
    with open(rc_overview_template_file_name) as fp:
        for line in fp:
            if "${{ contract }}" in line:
                rc_file.write(line.replace("${{ contract }}", contract_name))
            elif "${{ action }}" in line:
                for row in tr:
                    rc_file.write(row + "\n")
            else:
                rc_file.write(line)
    rc_file.close()

# generates a ricardian contract for each action from the action template
def generate_rc_action_files():
    tr = build_table_rows()
    rc_action_template_file_name = sys.argv[3]
    abi_file_name = sys.argv[1]
    contract_name = abi_file_name[:-4]
    for action in actions:
        rc_action_file_name = contract_name + "-" + action['name'] + '-rc.md'
        rc_file = open(rc_action_file_name,"w+")
        with open(rc_action_template_file_name) as fp:
            for line in fp:
                if "${{ action_name }}" in line:
                    rc_file.write(line.replace("${{ action_name }}", action['name']))
                elif "${{ action }}" in line:
                    for row in tr:
                        if action['name'] in row:
                            rc_file.write(row + "\n")
                else:
                    rc_file.write(line)
        rc_file.close()

# main program
def main():
    validate_input_arguments()
    get_actions_inputs_types()
    generate_rc_overview_file()
    generate_rc_action_files()

# runs main
if __name__== "__main__":
  main()