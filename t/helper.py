#! /usr/bin/env python3

import json
import requests
import subprocess
import time

from typing import List, Optional, Union

import color

def compress(s: str, length: int = 16, tail: int = 3):
    if len(s) > length:
        s = s[:(length - tail - 3)] + "..." + s[-tail:]
    return s


def extract(resp: requests.Response, key: str, fallback):
    try:
        return json.loads(resp.text)[key]
    except KeyError:
        return fallback


def fetch(data: dict, keys: List[str]) -> dict:
    return dict((k, data[k]) for k in keys)


def get_current_time(date=True, precision=3, local_time=False, time_zone=False):
    now = time.time()
    form = "%H:%M:%S"
    if date:
        form = "T".join(["%Y-%m-%d", form])
    if precision:
        subsec = int((now - int(now)) * (10 ** precision))
        form = ".".join([form, "{{:0{}d}}".format(precision).format(subsec)])
    if time_zone:
        form = " ".join([form, "%Z"])
    struct = time.localtime if local_time else time.gmtime
    return time.strftime(form, struct(now))


def header(text):
    return pad(color.decorate(text, fcolor="black", bcolor="cyan"))


# to be deprecated
def get_transaction_id(response: requests.Response) -> Optional[str]:
    try:
        return json.loads(response.text)["transaction_id"]
    except KeyError:
        return


def pretty_json(text: str, maxlen=100) -> str:
    data = json.loads(text)
    if maxlen:
        trim(data, maxlen=100)
    return json.dumps(data, indent=4, sort_keys=False)


def optional(x, y):
    return x if y is not None else None


def override(default_value, value, override_value=None):
    return override_value if override_value is not None else value if value is not None else default_value


def pad(text: str, left=10, total=100, right=None, char="-", sep=" ") -> str:
    """
    Summary
    -------
    This function provides padding for a string.

    Doctest
    -------
    >>> # implied_total (24) < total (25), so total becomes 24.
    >>> pad("hello, world", left=3, right=3, total=25, char=":", sep=" ~ ")
    '::: ~ hello, world ~ :::'
    """
    if right is not None:
        implied_total = len(char) * (left + right) + len(sep) * 2 + len(text)
        total = min(total, implied_total)
    return (char * left + sep + text + sep).ljust(total, char)


def pgrep(pattern: str) -> List[int]:
    out = subprocess.Popen(['pgrep', pattern], stdout=subprocess.PIPE).stdout.read()
    return [int(x) for x in out.splitlines()]


def trim(data, maxlen=100):
    """
    Summary
    -------
    A helper function for json formatting. This function recursively look
    into a nested dict or list that could be converted to a json string,
    and replace strings that are too long with three dots (...).
    """
    if isinstance(data, dict):
        for key in data:
            if isinstance(data[key], (list, dict)):
                trim(data[key])
            else:
                if isinstance(data[key], str) and len(data[key]) > maxlen:
                    data[key] = "..."
    elif isinstance(data, list):
        for item in data:
            if isinstance(item, (list, dict)):
                trim(item)
            else:
                if isinstance(item, str) and len(item) > maxlen:
                    item = "..."


def main():
    pass


if __name__ == '__main__':
    main()
