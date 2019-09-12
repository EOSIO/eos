#! /usr/bin/env python3

import json
import requests
import subprocess

from typing import List, Optional, Union


def extract(resp: requests.Response, key: str, fallback):
    try:
        return json.loads(resp.text)[key]
    except KeyError:
        return fallback


def fetch(data: dict, keys: List[str]) -> dict:
    return dict((k, data[k]) for k in keys)


def get_transaction_id(response: requests.Response) -> Optional[str]:
    try:
        return json.loads(response.text)["transaction_id"]
    except KeyError:
        return


def override(default_value, value):
    return default_value if value is None else value


def pgrep(pattern: str) -> List[int]:
    out = subprocess.Popen(['pgrep', pattern], stdout=subprocess.PIPE).stdout.read()
    return [int(x) for x in out.splitlines()]


def main():
    pass


if __name__ == '__main__':
    main()
