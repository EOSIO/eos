#!/usr/bin/env python3
from flask import Flask, request, jsonify
import json
import requests
import os
import socket
from urllib.parse import urlparse
import time
import logging
import click

log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

def secho(text, file=None, nl=None, err=None, color=None, **styles):
    pass

def echo(text, file=None, nl=None, err=None, color=None, **styles):
    pass

click.echo = echo
click.secho = secho

zipkin_url= os.environ['ZIPKIN_URL']
listen_port= os.environ.get('LISTEN_PORT', 9411)

trace_timestamps={}
traces=[]
hostnames={}

def get_hostname(addr):
   if addr in hostnames:
      return hostnames[addr]
   else:
      hostname=socket.gethostbyaddr(addr)[0].split(".")[0]
      hostnames[addr]=hostname
      return hostname

api = Flask(__name__)

@api.route('/api/v2/spans', methods=['POST'])
def post_span():
   spans = request.json
   for span in spans:
      trace_id = span["traceId"]
      timestamp = span["timestamp"]
      if "parentId" not in span:   
         trace_timestamps[trace_id] = timestamp
         traces.append(trace_id)
         if len(traces) > 360:
            trace_timestamps.pop(traces[0], None)
            traces.pop(0)
      if span["name"] == "process_received":
         if trace_id in trace_timestamps:
            latency=timestamp - trace_timestamps[trace_id] + span["duration"]
            address = span["localEndpoint"].get("ipv4", request.remote_addr)
            print("timestamp={} latency={} host={} traceID={}".format(timestamp, latency, get_hostname(address), trace_id), flush=True)
         else:
            print("trace not found remote_addr={} traceID={}".format(request.remote_addr, trace_id), flush=True)

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
