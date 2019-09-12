#! /usr/bin/env python3

import json
import requests
import select
import shutil
import sys

from helper import get_transaction_id
from typing import List, Optional, Union

__all__ = ["String", "Print", "pad"]


COLORS = {"black":          0,
          "red":            1,
          "green":          2,
          "yellow":         3,
          "blue":           4,
          "magenta":        5,
          "cyan":           6,
          "white":          7}

STYLES = {"reset":          0,
          "bold":           1,
          "faint":          2,
          "italic":         3,
          "underline":      4,
          "blink":          5,
          "fast-blink":     6,
          "reverse":        7,
          "hide":           8,
          "strikethrough":  9}


class String():
    """
    Summary
    -------
    This class offers methods that return a formatted or colorized string.
    """
    def __init__(self, invisible=False, monochrome=False):
        self.invisible = invisible
        self.monochrome = monochrome

    def decorate(self, text, style: Optional[Union[str, List[str]]] =None, fcolor: Optional[str] =None, bcolor: Optional[str] =None):
        attr = parse(style, fcolor, bcolor)
        return "" if self.invisible else text if self.monochrome else "\033[{}m{}\033[0m".format(attr, text) if attr else text

    def vanilla(self, text):
        return "" if self.invisible else text

    def bold(self, text):
        return self.decorate(text, style="bold")

    def underline(self, text):
        return self.decorate(text, style="underline")

    def reverse(self, text):
        return self.decorate(text, style="reverse")

    def red(self, text):
        return self.decorate(text, fcolor="red")

    def green(self, text):
        return self.decorate(text, fcolor="green")

    def yellow(self, text):
        return self.decorate(text, fcolor="yellow")

    def blue(self, text):
        return self.decorate(text, fcolor="blue")

    def magenta(self, text):
        return self.decorate(text, fcolor="magenta")

    def cyan(self, text):
        return self.decorate(text, fcolor="cyan")

    def white(self, text):
        return self.decorate(text, fcolor="white")


class Print():
    """
    Summary
    -------
    This class offers methods that extend print() function for format/color printing.
    """
    def __init__(self, invisible=False, monochrome=False):
        self.invisible = invisible
        self.monochrome = monochrome

    def decorate(self, *text, style: Optional[Union[str, List[str]]] =None, fcolor: Optional[str] =None, bcolor: Optional[str] =None, **kwargs):
        if self.invisible:
            return
        elif self.monochrome:
            print(*text, **kwargs)
        else:
            print("\033[{}m".format(parse(style, fcolor, bcolor)), end="")
            print(*text, end=(kwargs["end"] if "end" in kwargs else ""),
                  **(dict((k, kwargs[k]) for k in kwargs if k != "end")))
            print("\033[0m", end="" if "end" in kwargs else "\n")

    def vanilla(self, *text, **kwargs):
        if self.invisible:
            return
        else:
            print(*text, **kwargs)

    def bold(self, *text, **kwargs):
        return self.decorate(*text, style="bold", **kwargs)

    def underline(self, *text, **kwargs):
        return self.decorate(*text, style="underline", **kwargs)

    def reverse(self, *text, **kwargs):
        return self.decorate(*text, style="reverse", **kwargs)

    def red(self, *text, **kwargs):
        return self.decorate(*text, fcolor="red", **kwargs)

    def green(self, *text, **kwargs):
        return self.decorate(*text, fcolor="green", **kwargs)

    def yellow(self, *text, **kwargs):
        return self.decorate(*text, fcolor="yellow", **kwargs)

    def blue(self, *text, **kwargs):
        return self.decorate(*text, fcolor="blue", **kwargs)

    def magenta(self, *text, **kwargs):
        return self.decorate(*text, fcolor="magenta", **kwargs)

    def cyan(self, *text, **kwargs):
        return self.decorate(*text, fcolor="cyan", **kwargs)

    def white(self, *text, **kwargs):
        return self.decorate(*text, fcolor="white", **kwargs)

    def json(self, text, maxlen=100, func=None) -> None:
        if func is None:
            func = self.vanilla
        data = json.loads(text)
        if maxlen:
            self.trim(data, maxlen=100)
        func(json.dumps(data, indent=4, sort_keys=False))

    def transaction_id(self, response: requests.Response) -> None:
        tid = get_transaction_id(response)
        if tid:
            self.green("{:100}".format("<Transaction ID> {}".format(tid)))
        else:
            self.yellow("{:100}".format("Warning: No transaction ID returned."))

    def response_in_short(self, response: requests.Response) -> None:
        if response.ok:
            self.green(response, ' ' * 100)
        else:
            self.red(response)
            self.json(response.text)
        self.transaction_id(response)

    def response_in_full(self, response: requests.Response) -> None:
        self.green(response) if response.ok else self.red(response)
        self.json(response.text)
        self.transaction_id(response)

    def response_with_prompt(self, response: requests.Response, timeout=1) -> None:
        if response.ok:
            self.green(response, ' ' * 100)
            self.transaction_id(response)
            lines = json.dumps(json.loads(response.text), indent=4, sort_keys=False).count('\n') + 1
            self.reverse("--- Press [Enter] to view more ({} lines) ---".format(lines), end='\r')
            # if stdin is not empty within timeout, then print json in detail
            if select.select([sys.stdin], [], [], timeout)[0]:
                self.json(response.text)
                next(sys.stdin)
        else:
            self.red(response)
            self.json(response.text)

    @staticmethod
    def trim(data, maxlen=100):
        """
        Summary
        -------
        A helper function for json printing. This function recursively look
        into a nested dict or list that could be converted to a json string,
        and replace str values that are too long with three dots (...).
        """
        if isinstance(data, dict):
            for key in data:
                if isinstance(data[key], (list, dict)):
                    Print.trim(data[key])
                else:
                    if isinstance(data[key], str) and len(data[key]) > maxlen:
                        data[key] = "..."
        elif isinstance(data, list):
            for item in data:
                if isinstance(item, (list, dict)):
                    Print.trim(item)
                else:
                    if isinstance(item, str) and len(item) > maxlen:
                        item = "..."


def pad(text: str, left=10, right=None, total=None, char='-', sep=' ') -> str:
    """
    Summary
    -------
    This function provides padding for a string.

    Example
    -------
    >>> # implied_total (24) < total (25), so total becomes 24.
    >>> String().pad("hello, world", left=3, right=3, total=25, char=":", sep=' ~ ')
    '::: ~ hello, world ~ :::'
    """
    if total is None:
        total = shutil.get_terminal_size(fallback=(100, 20)).columns
    if right is not None:
        implied_total = len(char) * (left + right) + len(sep) * 2 + len(text)
        total = min(total, implied_total)
    return (char * left + sep + text + sep).ljust(total, char)


def parse(style, fcolor, bcolor):
    attr = [] if style is None else [STYLES[style]] if isinstance(style, str) else [STYLES[x] for x in style]
    if isinstance(fcolor, str):
        attr.append(30 + COLORS[fcolor])
    if isinstance(bcolor, str):
        attr.append(40 + COLORS[bcolor])
    return ";".join([str(x) for x in attr])


def test():
    """
    Doctest
    -------
    >>> Print().green("Hello", 3.14, ["a", "b", "c"], {"name": "Alice", "friends": ["Bob", "Charlie"]}, \
                      sep=";\\n", end=".\\n")
    \033[32mHello;
    3.14;
    ['a', 'b', 'c'];
    {'name': 'Alice', 'friends': ['Bob', 'Charlie']}.
    \033[0m
    """
    Print().green("Run with option -v to view doctest details.")


if __name__ == "__main__":
    import doctest
    doctest.testmod()
    test()
