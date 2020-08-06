#! /usr/bin/env python3

# standard libraries
import copy
import json
import shlex
import subprocess
import time
import typing
# user-defined modules
if __package__:
    from . import color
    from . import logger
else:
    import color
    import logger

# --------------- utility ---------------------------------------------------------------------------------------------

def override(*args):
    """
    Example
    -------
    >>> a, b, c = 1, 2, None
    >>> override(a, b, c)
    2
    >>> default_value = 1
    >>> value = 2
    >>> command_line_value = 3
    >>> value = override(default_value, value, command_line_value)
    >>> value
    3
    """
    for x in reversed(args):
        if x is not None:
            return x
    return None

# --------------- format ----------------------------------------------------------------------------------------------

def get_time(date=True, precision=3, local_time=False, time_zone=False):
    now = time.time()
    fmt = "%H:%M:%S"
    if date:
        fmt = "T".join(["%Y-%m-%d", fmt])
    if precision:
        subsec = int((now - int(now)) * (10 ** precision))
        fmt = ".".join([fmt, f"{subsec:0{precision}d}"])
    if time_zone:
        fmt = " ".join([fmt, "%Z"])
    convert = time.localtime if local_time else time.gmtime
    return time.strftime(fmt, convert(now))


def make_tokens(amount: int, ndigits=4, symbol="SYS"):
    """
    Doctest
    -------
    >>> print(make_tokens(37.5e6))
    37500000.0000 SYS
    >>> print(make_tokens(1, 3, 'ABC'))
    1.000 ABC
    """
    return "{{:.{}f}} {{}}".format(ndigits).format(amount, symbol)

# --------------- string ----------------------------------------------------------------------------------------------

def plural(word, count, suffix="s"):
    if isinstance(word, tuple) or isinstance(word, list):
        return word[0] if count <= 1 else word[1]
    else:
        return word if count <= 1 else  word + suffix


def singular(word, count, suffix="s"):
    if isinstance(word, tuple) or isinstance(word, list):
        return word[0] if count <= 1 else word[1]
    else:
        return word  + suffix if count <= 1 else  word


def pad(text, total, left=0, char=" ", sep="", textlen=None) -> str:
    """textlen is a hint for visible length of text"""
    textlen = vislen(text) if textlen is None else textlen
    offset = len(text) - textlen
    return (char * left + sep + text + sep).ljust(total + offset, char)


def vislen(s: str):
    """Return visible length of a string after removing color codes in it."""
    return len(color.REGEX.sub("", s))


def squeeze(s, maxlen, tail=0):
    if len(s) <= maxlen:
        return s
    return s[:(maxlen-tail-2)] + ".." + (s[-tail:] if tail else "")

# --------------- json ------------------------------------------------------------------------------------------------

def abridge(data: typing.Union[dict, list], maxlen=79):
    clone = copy.deepcopy(data)
    trim(clone, maxlen=maxlen)
    return clone


def trim(data: typing.Union[dict, list], maxlen=79):
    """
    Summary
    -------
    A helper function for json formatting. This function recursively looks
    into a nested dict or list that could be converted to a json string,
    and replace strings that are too long with three dots (...).
    """
    if isinstance(data, dict):
        for key in data:
            if isinstance(data[key], (list, dict)):
                trim(data[key], maxlen=maxlen)
            else:
                if isinstance(data[key], str) and len(data[key]) > maxlen:
                    data[key] = squeeze(data[key], maxlen=maxlen, tail=3)
    elif isinstance(data, list):
        for item in data:
            if isinstance(item, (list, dict)):
                trim(item, maxlen=maxlen)
            else:
                if isinstance(item, str) and len(item) > maxlen:
                    item = squeeze(item, maxlen=maxlen, tail=3)

# --------------- subprocess ------------------------------------------------------------------------------------------

class ServiceInfo:
    def __init__(self, pid: int, file: str, port: int, gene: str):
        self.pid = pid
        self.file = file
        self.port = port
        self.gene = gene


def get_service_list_by_cmd(cmd: str) -> typing.List[ServiceInfo]:
    pid_list = sorted([int(x) for x in run(f"pgrep -f {cmd}")])
    service_list = []
    port_set = set()
    for pid in pid_list:
        try:
            cmd_and_args = run(f"ps -p {pid} -o command=")[0]
        except IndexError:
            continue
        file = port = gene = None
        for ind, val in enumerate(shlex.split(cmd_and_args)):
            if ind == 0:
                file = val
            elif val.startswith("--http-server-address"):
                port = int(val.split(":")[-1])
            elif val.startswith("--genesis-file"):
                gene = val.split("=")[-1]
        if port not in port_set:
            service_list.append(ServiceInfo(pid, file, port, gene))
            port_set.add(port)
    return service_list


def terminate(pid: typing.Union[int, str]):
    return run(f"kill -SIGTERM {pid}")


def run(args: typing.Union[str, typing.List[str]]):
    if isinstance(args, str):
        args = args.split(" ")
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True).stdout.splitlines()

# --------------- doctest ---------------------------------------------------------------------------------------------

if __name__ == '__main__':
    import doctest
    doctest.testmod()
    print("Pass -v in command line for details of doctest.")
