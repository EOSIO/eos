#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import os
import xml.etree.cElementTree as ET
Element = type(ET.Element(None))

from typename_conversion_table import typenameConversion

class MemberdefEncoder(json.JSONEncoder):

  def default(self, obj):
    if isinstance(obj, Element):
      properties = {}
      node = obj.find('./type/ref')
      if node is not None: # Explicit test for None since the truth value of an instance of Element is False.
        if node.get('kindref') == 'compound':
          properties['results'] = self.loadAndParseCompoundType(node.get('refid'))
        elif node.get('kindref') == 'member':
          # Can't do this.  Must start from root node.
          # Total rearchitecting required.  Rip apart MemberdefEncoder, doing none of this in it.
          #  Don't even use an instance of it.  It can't do what's necessary parsing the XML.
          #print "../..//memberdef[@id='%s']" % node.get('refid')
          #typeNode = obj.find("../..//memberdef[@id='%s']" % node.get('refid'))
          #print 'Parent node is', obj.find("./..").tag
          #print 'Type node is', typeNode
          properties['results'] = {} # !!TODO!!
      for node in obj.findall('./param'):
        # EOS code currently uses a single structure for all method arguments
        paramType = node.find('./type/ref').get('kindref')
        # Some are included structures, identified as 'compound'.
        if paramType == 'compound':
          paramName = node.find('declname').text.strip()
          properties[paramName] = self.loadAndParseCompoundType(node.find('./type/ref').get('refid'))
        # Others are members
        elif paramType == 'member':
          paramNode = node.find('declname')
          if paramNode is not None:
            paramName = paramNode.text.strip()
          else:
            paramName = ''
          properties[paramName] = {} # !!TODO!!
      node = obj.find('./briefdescription/para')
      if node is not None:
        properties['brief'] = ''.join(node.itertext()).strip()
      else:
        properties['brief'] = ''
      return properties
    return json.JSONEncoder.default(self, obj)

  def loadAndParseCompoundType(self, refid):
    results = {}
    filename = os.path.join('../doxygen/xml/', refid + '.xml')
    #print 'Loading', filename
    typeRoot = ET.parse(filename).getroot()
    bases = typeRoot.findall('.//basecompoundref')
    for base in bases:
      results.update(self.loadAndParseCompoundType(base.get('refid')))
    for argMember in typeRoot.findall('.//memberdef'):
      if argMember.get('prot') != 'public':
        continue
      if argMember.get('kind') not in ['variable', 'typedef']:
        continue
      memberVars = {}
      varType = argMember.find('./type')
      if varType is not None:
        varStr = ''.join(varType.itertext()).strip()
        if varStr in typenameConversion:
          memberVars['type'] = typenameConversion[varStr]
        else:
          memberVars['type'] = varStr
      initializer = argMember.find('./initializer')
      if initializer is not None:
        initializerStr = ''.join(initializer.itertext())[1:].strip()
        if 'uint64_t' in initializerStr:
          initializerStr = initializerStr[len('uint64_t('):-1]
        if 'uint128_t' in initializerStr:
          initializerStr = initializerStr[len('uint128_t('):-1]
        memberVars['default'] = initializerStr
      desc = argMember.find('./briefdescription/para')
      if desc is not None:
        memberVars['doc'] = ''.join(desc.itertext()).strip()
      else:
        memberVars['doc'] = ''
      name = argMember.find('./name')
      if name is not None:
        nameKey = ''.join(name.itertext()).strip()
        results[nameKey] = memberVars
    return results

def parseMemberFunctions(root, members, omit):
  for member in root.findall('.//memberdef'):
    if member.get('kind') == 'function' and member.get('prot') == 'public':
      memberName = member.find('./name')
      if memberName.text != omit: # Omit constructor. (Usually. Could be anything.)
        members[memberName.text] = member

root = ET.parse('../doxygen/xml/classeos_1_1chain__apis_1_1read__only.xml').getroot()

members = {}

parseMemberFunctions(root, members, 'read_only')

root = ET.parse('../doxygen/xml/classeos_1_1chain__apis_1_1read__write.xml').getroot()

parseMemberFunctions(root, members, 'read_write')

print json.dumps(members, cls=MemberdefEncoder, indent=2, separators=(',', ': '), sort_keys=True)
