#!/usr/bin/env bash
#
# This script captures the generic fuzz testing to which
# net_plugin has been subjected.  It typically results in
# message buffers in excess of 1 gigabyte.
#
if ! pgrep aos > /dev/null; then
   echo "Run aOS with net_plugin configured for port 6620."
   exit 1
fi
for i in `seq 1 10000`; do netcat localhost 6620 < /dev/urandom; done
