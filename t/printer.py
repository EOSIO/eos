import json
import requests
import select
import shutil
import sys

class Colors:
    RESET = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    BLUE = '\033[34m'

def print_in_red(s: str):
    print(Colors.RED, s, Colors.RESET, sep='')

def print_in_green(s: str):
    print(Colors.GREEN, s, Colors.RESET, sep='')

def print_in_blue(s: str):
    print(Colors.BLUE, s, Colors.RESET, sep='')

def print_in_color(s: str, red=None, green=None, blue=None):
    if red:
        print_in_red(s)
    elif green:
        print_in_green(s)
    elif blue:
        print_in_blue(s)
    else:
        print_in_red(s)

def print_response(response: requests.Response, timeout=1, verbosity=1) -> None:
    print_in_color(response, green=response.ok)
    print("Press any key to view more ...", end='\r')
    i, o, e = select.select([sys.stdin], [], [], timeout)
    if i or verbosity >= 2:
        print('\n', json.dumps(json.loads(response.text), indent=4, sort_keys=True))
        next(sys.stdin)

def pad(s:str, left=10, total=None, char='-', sep=' ')-> str:
    if total is None:
        total = shutil.get_terminal_size(fallback=(80, 20)).columns
    return (char * left + sep + s + sep).ljust(total, char)
