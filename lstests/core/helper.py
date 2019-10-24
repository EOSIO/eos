#! /usr/bin/env python3

import copy
import json
import re
import subprocess
import time
import typing

from typing import List, Optional, Union

# third-party libraries
import requests

# user-defined modules
import color

HORIZONTAL_BAR = "─"
HORIZONTAL_DASH = "⎯"
VERTICAL_BAR = "│"


def vislen(s: str):
    """return real visual length of a str after removing color code"""
    return len(re.compile(r"\033\[[0-9]+(;[0-9]+)?m").sub("", s))


def compress(s: str, length: int = 16, ellipsis="..", tail: int = 0):
    if len(s) <= length:
        return s
    return s[:(length - tail - 2)] + ".." + (s[-tail:] if tail else "")


# todo: review usage
def extract(resp: requests.models.Response, key: str, fallback):
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


def format_header(header, level: str):
    upper = level.upper()
    if upper == "INFO":
        return color.bold(">>> {}".format(header))
    if upper == "DEBUG":
        return pad(color.black_on_cyan(header), char="-")
    if upper == "TRACE":
        return pad(header, char="⎯")
    raise RuntimeError("Invalid header level \"{}\"".format(level))


# to be deprecated
def get_transaction_id(response: requests.models.Response) -> Optional[str]:
    try:
        return json.loads(response.text)["transaction_id"]
    except KeyError:
        return

def format_json(text: str, maxlen=100) -> str:
    data = json.loads(text)
    if maxlen:
        trim(data, maxlen=100)
    return json.dumps(data, indent=4, sort_keys=False)


def format_tokens(amount: int, ndigits=4, symbol="SYS"):
    """
    Doctest
    -------
    >>> print(format_tokens(37.5e6))
    37500000.0000 SYS
    >>> print(format_tokens(1, 3, 'ABC'))
    1.000 ABC
    """
    return "{{:.{}f}} {{}}".format(ndigits).format(amount, symbol)




def optional(x, y):
    return x if y is not None else None


# def override(default_value, value, override_value=None):
#     return override_value if override_value is not None else value if value is not None else default_value

def override(*args):
    for x in reversed(args):
        if x is not None:
            return x
    return None


def pad(text: str, left=20, total=90, right=None, char="-", sep=" ") -> str:
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
        implied_total = vislen(char) * (left + right) + vislen(sep) * 2 + vislen(text)
        total = min(total, implied_total)
    string = char * left + sep + text + sep
    offset = len(string) - vislen(string)
    return string.ljust(total + offset, char)


def abridge(data: typing.Union[dict, list], maxlen=64):
    clone = copy.deepcopy(data)
    trim(clone, maxlen=maxlen)
    return clone


def trim(data: typing.Union[dict, list], maxlen=64):
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

# --------------- subprocess-related ----------------------------------------------------------------------------------

def get_cmd_and_args_by_pid(pid: typing.Union[int, str]):
    return interacitve_run(["ps", "-p", str(pid), "-o", "command="])


def get_pid_list_by_pattern(pattern: str) -> typing.List[int]:
    out = interacitve_run(["pgrep", "-f", pattern])
    return [int(x) for x in out.splitlines()]


def interacitve_run(args: list):
    return subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True).stdout.read().rstrip()


def quiet_run(args: list) -> None:
    subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def terminate(pid: typing.Union[int, str]) -> None:
    quiet_run(["kill", "-SIGTERM", str(pid)])

# --------------- test ------------------------------------------------------------------------------------------------

def test():
    print("Pass -v in command line for details of doctest.")


if __name__ == '__main__':
    import doctest
    doctest.testmod()
    test()
