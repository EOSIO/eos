#!/usr/bin/env python3

import base64
import getpass
import hashlib
import json
import os

pw = getpass.getpass("enter your password:  ")
pw_bytes = pw.encode("utf-8")
salt_bytes = os.urandom(8)
salt_b64 = base64.b64encode( salt_bytes )
pw_hash = hashlib.sha256( pw_bytes + salt_bytes ).digest()
pw_hash_b64 = base64.b64encode( pw_hash )

print(json.dumps(
{
    "password_hash_b64" : pw_hash_b64.decode("ascii"),
    "password_salt_b64" : salt_b64.decode("ascii"),
},
sort_keys=True,
indent=3, separators=(',', ' : ')
))
