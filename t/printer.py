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
    YELLOW = '\033[33m'
    BLUE = '\033[34m'

def print_in_red(printable):
    print(Colors.RED, printable, Colors.RESET, sep='')

def print_in_green(printable):
    print(Colors.GREEN, printable, Colors.RESET, sep='')

def print_in_blue(printable):
    print(Colors.BLUE, printable, Colors.RESET, sep='')

def print_in_color(printable, red=None, green=None, blue=None):
    if red:
        print_in_red(printable)
    elif green:
        print_in_green(printable)
    elif blue:
        print_in_blue(printable)
    else:
        print_in_red(printable)

def trim_json(d, maxlen=100):
    if isinstance(d, dict):
        for key in d:
            if not isinstance(d[key], (list, dict)):
                if isinstance(d[key], str) and len(d[key]) > maxlen:
                    d[key] = "..."
            else:
                trim_json(d[key])
    elif isinstance(d, list):
        for item in d:
            if not isinstance(item, (list, dict)):
                if isinstance(item, str) and len(item) > maxlen:
                    item = "..."
            else:
                trim_json(item)

def print_json(text, maxlen=100):
    d = json.loads(text)
    if maxlen:
        trim_json(d, maxlen=100)
    print(json.dumps(d, indent=4, sort_keys=True))

def print_response(response: requests.Response, timeout=1, verbosity=1) -> None:
    if response.ok:
        print_in_green(response)
        if verbosity == 2:
            print_json(response.text, maxlen=0)
        if verbosity == 1:
            print("Press [Enter] to view more ...", end='\r')
            if select.select([sys.stdin], [], [], timeout)[0]:
                print_json(response.text)
                next(sys.stdin)
    else:
        print_in_red(response)
        print_json(response.text)

def red_str(string: str):
    return Colors.RED + string + Colors.RESET

def green_str(string: str):
    return Colors.GREEN + string + Colors.RESET

def yellow_str(string: str):
    return Colors.YELLOW + string + Colors.RESET

def blue_str(string: str):
    return Colors.BLUE + string + Colors.RESET

def underlined_str(string: str):
    return Colors.UNDERLINE + string + Colors.RESET

def pad(s:str, left=10, total=None, char='-', sep=' ')-> str:
    if total is None:
        total = shutil.get_terminal_size(fallback=(80, 20)).columns
    return (char * left + sep + s + sep).ljust(total, char)
