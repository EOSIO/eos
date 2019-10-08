#! /usr/bin/env python3

# standard libraries
import json
import requests

# user-defined modules
import helper


class Request:
    def __init__(self, url: str, data: str):
        self.url = url
        self.data = data

    def post(self):
        return requests.post(self.url, data=self.data)


class Connection:
    def __init__(self, endpoint, service, data: dict, dont_attempt=False):
        self.request = Request(self.get_url(endpoint, service.address, service.port), json.dumps(data))
        if not dont_attempt:
            self.attempt()


    def attempt(self):
        self.response = self.request.post()
        self.transaction_id = helper.extract(self.response, key="transaction_id", fallback=None)
        return self.response, self.transaction_id


    @staticmethod
    def get_url(endpoint, address, port):
        return "http://{}:{}/v1/launcher/{}".format(address, port, endpoint)
