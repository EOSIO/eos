#! /usr/bin/env python3

# standard libraries
import json
import requests

# user-defined modules
import helper
import color


class Connection:
    def __init__(self, url, data: dict, dont_attempt=False):
        self.url = url
        self.data = data
        if not dont_attempt:
            self.attempt()


    def attempt(self):
        self.response = requests.post(self.url, json=self.data)


    @property
    def request_dict(self):
        return self.data


    @property
    def request_text(self):
        return json.dumps(self.data, indent=4)


    @property
    def ok(self):
        if not hasattr(self, "response"):
            return False
        return self.response.ok


    @property
    def response_code(self):
        if not hasattr(self, "response"):
            return color.red("<No Response>")
        return color.green(self.response) if self.response.ok else color.red(self.response)


    @property
    def response_dict(self):
        if not hasattr(self, "response"):
            return dict()
        if not hasattr(self.response, "_response_dict"):
            self._response_dict = self.response.json()
        return self._response_dict


    @property
    def response_text_unabridged(self):
        if not hasattr(self, "response"):
            return str()
        if not hasattr(self.response, "_response_text_unabridged"):
            self._response_text_unabridged = json.dumps(self._response_dict, indent=4)
        return self._response_text_unabridged


    @property
    def response_text(self):
        if not hasattr(self, "response"):
            return str()
        if not hasattr(self.response, "_response_text"):
            self._response_dict_abridged = helper.abridge(self.response_dict)
            self._response_text = json.dumps(self._response_dict_abridged, indent=4)
        return self._response_text


    @property
    def transaction_id(self):
        if not hasattr(self, "response"):
            return str()
        if not hasattr(self, "_transaction_id"):
            if "transaction_id" in self.response_dict:
                self._transaction_id = self.response_dict["transaction_id"]
            else:
                self._transaction_id = str()
        return self._transaction_id
