#! /usr/bin/env python3

# standard libraries
import json
import requests

# user-defined modules
import helper
import color


class Request:
    def __init__(self, url: str, data: str):
        self.url = url
        self.data = data

    def post(self):
        return requests.post(self.url, data=self.data)


class Interaction:
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


    def get_formatted_response(self, show_content=False):
        res = []
        res.append(color.green(self.response) if self.response.ok else color.red(self.response))
        if show_content or not self.response.ok:
            res.append(helper.format_json(self.response.text))
        if self.transaction_id:
            res.append(color.green("<Transaction ID> {}".format(self.transaction_id)))
        return "\n".join(res)


def response_in_full(self, response: requests.Response) -> None:
    self.green(response) if response.ok else self.red(response)
    self.json(response.text)
    self.transaction_id(response)
