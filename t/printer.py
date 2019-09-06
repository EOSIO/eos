#! /usr/bin/env python3

import json
import requests
import select
import shutil
import sys

__all__ = ["String", "Print"]


CODES = {"reset":       "\033[0m",
         "bold":        "\033[1m",
         "underline":   "\033[4m",
         "reverse":     "\033[7m",
         "red":         "\033[31m",
         "green":       "\033[32m",
         "yellow":      "\033[33m",
         "blue":        "\033[34m",
         "purple":      "\033[35m",
         "cyan":        "\033[36m",
         "white":       "\033[37m"}


class String():
    """
    Summary
    -------
    This class offers methods that return a formatted or colorized string.
    """
    def __init__(self, invisible=False, monochrome=False):
        self.invisible = invisible
        self.monochrome = monochrome

    def decorate(self, text, style):
        return "" if self.invisible else text if self.monochrome else CODES[style] + text + CODES["reset"]

    def vanilla(self, text):
        return "" if self.invisible else text

    def bold(self, text):
        return self.decorate(text, "bold")

    def underline(self, text):
        return self.decorate(text, "underline")

    def reverse(self, text):
        return self.decorate(text, "underline")

    def red(self, text):
        return self.decorate(text, "red")

    def green(self, text):
        return self.decorate(text, "green")

    def yellow(self, text):
        return self.decorate(text, "yellow")

    def blue(self, text):
        return self.decorate(text, "blue")

    def purple(self, text):
        return self.decorate(text, "purple")

    def cyan(self, text):
        return self.decorate(text, "cyan")

    def white(self, text):
        return self.decorate(text, "white")

    @staticmethod
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


class Print():
    """
    Summary
    -------
    This class offers methods that extend print() function for format/color printing.
    """
    def __init__(self, invisible=False, monochrome=False):
        self.invisible = invisible
        self.monochrome = monochrome

    def decorate(self, *text, style, **kwargs):
        if self.invisible:
            return
        elif self.monochrome:
            print(*text, **kwargs)
        else:
            print(CODES[style], end="")
            print(*text, end=(kwargs["end"] if "end" in kwargs else ""),
                  **(dict((k, kwargs[k]) for k in kwargs if k != "end")))
            print(CODES["reset"], end="" if "end" in kwargs else "\n")

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
        return self.decorate(*text, style="red", **kwargs)

    def green(self, *text, **kwargs):
        return self.decorate(*text, style="green", **kwargs)

    def yellow(self, *text, **kwargs):
        return self.decorate(*text, style="yellow", **kwargs)

    def blue(self, *text, **kwargs):
        return self.decorate(*text, style="blue", **kwargs)

    def purple(self, *text, **kwargs):
        return self.decorate(*text, style="purple", **kwargs)

    def cyan(self, *text, **kwargs):
        return self.decorate(*text, style="cyan", **kwargs)

    def white(self, *text, **kwargs):
        return self.decorate(*text, style="white", **kwargs)

    def json(self, text, maxlen=100, func=None) -> None:
        if func is None:
            func = self.vanilla
        data = json.loads(text)
        if maxlen:
            self.trim(data, maxlen=100)
        func(json.dumps(data, indent=4, sort_keys=False))

    def response_in_short(self, response: requests.Response, timeout=1) -> None:
        if response.ok:
            self.green(response, ' ' * 100)
            self.reverse("--- Press [Enter] to view more ---", end='\r')
            # if stdin is not empty within timeout, then print json in detail
            if select.select([sys.stdin], [], [], timeout)[0]:
                self.json(response.text)
                next(sys.stdin)
        else:
            self.red(response)
            self.json(response.text)

    def response_in_full(self, response: requests.Response) -> None:
        self.green(response) if response.ok else self.red(response)
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
