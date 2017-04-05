#!/usr/bin/env python3

import json
import os
import re
import xml.etree.ElementTree as etree

def process_node(path, node):
    """
    if node.tag == "TestCase":
        if node.attrib.get("result", "UNKNOWN") != "passed":
            failset.add(node)
        return
    if node.tag in ["TestResult", "TestSuite"]:
        for child in node:
            cpath = path+"/"+child.attrib["name"]
            process_node(cpath, child)
        return
    """
    #print("unknown node", node.tag)
    print(node.tag)
    return

name2members_doxygen = {}

def process_class_node(node):
    result = {"name":"", "vmembers":[]}
    for child in node:
        if child.tag == "name":
            result["name"] = child.text
        elif child.tag == "member":
            kind = child.attrib.get("kind")
            if kind == "variable":
                result["vmembers"].append(child[0].text)
    name2members_doxygen[result["name"]] = result["vmembers"]
    return

tree = etree.parse("doxygen/xml/index.xml")
root = tree.getroot()
for child in root:
    if (child.tag == "compound") and (child.attrib.get("kind") in ["struct", "class"]):
        process_class_node(child)

s_static_names = set(["space_id", "type_id"])

for k, v in name2members_doxygen.items():
    name2members_doxygen[k] = [x for x in v if x not in s_static_names]

#with open("stuff/member_enumerator.out", "r") as f:
#    name2members_fc = json.load(f)

# scan for FC_REFLECT( eos::... in all cpp,hpp files under libraries/ programs/ tests/

re_reflect = re.compile(r"""
FC_REFLECT\s*[(]
\s*(eos::[a-zA-Z0-9_:]+)
\s*,
((?:\s*[(]\s*[a-zA-Z0-9_]+\s*[)])*)
""", re.VERBOSE)

re_reflect_derived = re.compile(r"""
FC_REFLECT_DERIVED\s*[(]
\s*(eos::[a-zA-Z0-9_:]+)
\s*,
\s*[(]\s*(eos::[a-zA-Z0-9_:]+)\s*[)]
\s*,
((?:\s*[(]\s*[a-zA-Z0-9_]+\s*[)])*)
""", re.VERBOSE)

re_bubble_item = re.compile(r"\s*[(]\s*([a-zA-Z0-9_]+)\s*")

def bubble_list(x):
    return [re_bubble_item.match(e).group(1) for e in x.split(")")[:-1]]

name2members_re = {}

for root, dirs, files in os.walk("."):
    if root == ".":
        dirs[:] = ["libraries", "programs", "tests"]
    for filename in files:
        if not (filename.endswith(".cpp") or filename.endswith(".hpp")):
            continue
        try:
            with open( os.path.join(root, filename), "r" ) as f:
                content = f.read()
                for m in re_reflect.finditer(content):
                    cname = m.group(1)
                    members = bubble_list(m.group(2))
                    name2members_re[cname] = members
                    if cname.endswith("_object"):
                       print("FC_REFLECT on {} should be FC_REFLECT_DERIVED".format(cname))
                for m in re_reflect_derived.finditer(content):
                    cname = m.group(1)
                    members = bubble_list(m.group(3))
                    name2members_re[cname] = members
        except OSError as e:
            pass

def validate_members(name2members_ref, name2members_test):
    ok_items = []
    ne_items = []
    error_items = []

    for name in sorted(name2members_ref.keys()):
        if name not in name2members_test:
            ne_items.append(name)
        elif sorted(name2members_ref[name]) != sorted(name2members_test[name]):
            error_items.append(name)
            print("")
            print("error in", name)
            print("doxygen:", name2members_ref[name])
            print("fc     :", name2members_test[name])
        else:
            ok_items.append(name)
    return

"""
print("")
print("ok:")
for item in ok_items:
    print(item)

print("")
print("not evaluated:")
for item in ne_items:
    print(item)

print("")
print("error:")
for item in error_items:
    print(item)
"""

validate_members(name2members_doxygen, name2members_re)
