#! /usr/bin/env python3

"""core.connection - HTTP connection for communication with launcher service

This module provides a Connection class that handles HTTP connection with the
Launcher Service. It wraps around `requests` library (Apache 2.0 License), and
works with a request-response model.
"""

# standard libraries
import json
# third-party library
import requests
# user-defined modules
if __package__:
    from . import helper
    from . import color
else:
    import color
    import helper


class Connection:
    """An HTTP connection with request/response to and from launcher service.

    In general, the request and response are provided as read-only properties,
    in two formats:
    (1) text format (str type in json string format)
    (2) dict format (dict type)
    """
    def __init__(self, url: str, data: dict, dont_attempt=False):
        """Create a Connection with request URL and data.

        Unless explicitly setting `dont_attempt=True`, the request will be
        posted automatically when the Connection object is created.

        Parameters
        ----------
        url : int
            HTTP request URL.
        data : dict
    `       HTTP request data.
        dont_attempt : bool, optional
            If True, do not attempt to post the request at creation.
        """
        self.url = url
        self.data = data
        if not dont_attempt:
            self.attempt()

    def attempt(self) -> None:
        """Post the HTTP request and store the response in the object."""
        self.response = requests.post(self.url, json=self.data)

    @property
    def request_dict(self) -> dict:
        """Returns HTTP request data in dict type."""
        return self.data

    @property
    def request_text(self) -> str:
        """Returns HTTP request data in str type (json string format)."""
        return json.dumps(self.data, indent=4)

    @property
    def ok(self) -> bool:
        """Returns True if object has an OK response (code 200)."""
        if not hasattr(self, "response"):
            return False
        return self.response.ok

    @property
    def response_code(self) -> str:
        """Returns colored HTTP response code (e.g. 200) in str type.

        If no response, returns "<No Response>" in red."""
        if not hasattr(self, "response"):
            return color.red("<No Response>")
        if self.response.ok:
            return color.green(self.response)
        else:
            return color.red(self.response)

    @property
    def response_dict(self) -> dict:
        """Returns HTTP response in dict type."""
        if not hasattr(self, "response"):
            return {}
        if not hasattr(self.response, "_response_dict"):
            self._response_dict = self.response.json()
        return self._response_dict

    @property
    def response_text_unabridged(self) -> str:
        """Returns full HTTP response text in str type (json string format)."""
        if not hasattr(self, "response"):
            return ""
        if not hasattr(self.response, "_response_text_unabridged"):
            self._response_text_unabridged = json.dumps(self._response_dict,
                                                        indent=4)
        return self._response_text_unabridged

    @property
    def response_text(self) -> str:
        """Returns HTTP response in str type (json string format).

        The response text is abridged -- strings that are too long (e.g.
        content of a smart contract in hex format) are trimmed."""
        if not hasattr(self, "response"):
            return ""
        if not hasattr(self.response, "_response_text"):
            self._response_dict_abridged = helper.abridge(self.response_dict,
                                                          maxlen=79)
            self._response_text = json.dumps(self._response_dict_abridged,
                                             indent=4)
        return self._response_text

    @property
    def transaction_id(self) -> str:
        """Returns transaction ID in str type."""
        if not hasattr(self, "response"):
            return ""
        if not hasattr(self, "_transaction_id"):
            if "transaction_id" in self.response_dict:
                self._transaction_id = self.response_dict["transaction_id"]
            else:
                self._transaction_id = ""
        return self._transaction_id
