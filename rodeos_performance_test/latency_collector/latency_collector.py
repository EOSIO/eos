#!/usr/bin/env python3
from flask import Flask, request, jsonify
import json
import requests
import os
import socket
from urllib.parse import urlparse
import time

zipkin_url= os.environ['ZIPKIN_URL']
listen_port= os.environ.get('LISTEN_PORT', 9411)

trace_timestamps={}

api = Flask(__name__)

@api.route('/api/v2/spans', methods=['POST'])
def post_span():
   spans = request.json
   for span in spans:
      if "parentId" not in span:
         trace_timestamps[span["traceId"]] = span["timestamp"]
      if span["name"] == "process_received" and span["traceId"] in trace_timestamps:
         latency=span["timestamp"] - trace_timestamps[span["traceId"]] + span["duration"]
         hostname=socket.gethostbyaddr(request.remote_addr)[0]
         print("timestamp={} latency={} host={} traceID={}".format(span["timestamp"], latency, hostname, span["traceId"]))
   headers = {'Content-type': 'application/json'}
   r = requests.post(zipkin_url , headers=headers, data = request.data)
   return r.text, r.status_code

if __name__ == '__main__':
   o = urlparse(zipkin_url)
   while True:
      try:
         s = socket.socket()
         s.connect((o.hostname, o.port))
         break
      except:
         print("Unable to connect to {}:{}, wait for another 3 seconds".format(o.hostname, o.port))
         time.sleep(3)

   api.run(host='0.0.0.0', port=listen_port)
