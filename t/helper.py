#! /usr/bin/env python3

import json
import requests

from typing import List, Optional, Union


def fetch(data: dict, keys: List[str]) -> dict:
    return dict((k, data[k]) for k in keys)


def get_transaction_id(response: requests.Response) -> Optional[str]:
    try:
        return json.loads(response.text)["transaction_id"]
    except KeyError:
        return


def main():
    pass


if __name__ == '__main__':
    main()
