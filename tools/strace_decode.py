#!/usr/bin/env python3

import sys
import subprocess
from pprint import pprint

class LineInfo:
    full = ''
    num  = 0
    addr = ''
    exe  = ''


class Addr2LineDecoder:
    args   = []
    result = {}
    addr   = ''
    source = ''

    def __init__(self, exe):
        self.args = ['/usr/bin/addr2line', '-Cfipe', exe, '-a']

    def add_addr(self, addr):
        self.args.append(addr)

    def add_source(self):
        if self.addr != '':
            self.result[self.addr] = self.source.rstrip('\r\n')
        self.addr    = ''
        self.source  = ''

    def decode_addrs(self):
        proc = subprocess.Popen(self.args, shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        self.result = {}
        self.addr   = ''
        self.source = ''

        for line in proc.stdout.readlines():
            line = line.decode('ascii')
            if line.startswith('0x'):
                self.add_source()

                pos = line.index(': ')
                self.addr   = line[0: pos]
                self.source = line[pos + 2: len(line)]
            else:
                self.source += line

        self.add_source()


class Addr2LineMap:
    decoders = {}

    def add_addr(self, exe, addr):
        self.decoders.setdefault(exe, Addr2LineDecoder(exe)).add_addr(addr)

    def decode_addrs(self):
        for dec in self.decoders.values():
            dec.decode_addrs()

    def get_addr(self, exe, addr):
        return self.decoders[exe].result[addr.lower()]


strace = []
addr2line = Addr2LineMap()

for line in sys.stdin:
    words = line.strip().split(' ')

    info = LineInfo()
    info.full = line.rstrip('\r\n')
    if len(words) > 2 and words[0].endswith('#') and words[1].startswith('0x') and words[2] == 'in':
        info.num  = int(words[0].strip('#'))
        info.exe  = words[3]
        info.addr = words[1]
        addr2line.add_addr(info.exe, info.addr)

    strace.append(info)

addr2line.decode_addrs()
print('')
for info in strace:
    if info.exe == '':
        print(info.full)
    else:
        print('{:2}# {} in {}'.format(info.num, addr2line.get_addr(info.exe, info.addr), info.exe))
