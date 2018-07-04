#!/usr/bin/env bash
#
# This script captures the generic fuzz testing to which
# net_plugin has been subjected.  It typically results in
# message buffers in excess of 1 gigabyte.
#
if ! pgrep nodeos > /dev/null; then
   echo "Run nodeos with net_plugin configured for port 9876."
   exit 1
fi
for i in `seq 1 10000`; do netcat localhost 9876 < /dev/urandom; done
