#!/bin/bash

programs/launcher/launcher -p 1
sleep 10
programs/launcher/launcher -k 9
count=`grep -c "generated block" tn_data_0/stderr.txt`
if [ $count == 0 ]; then
  echo FAILURE - no blocks produced
  exit 1
fi
echo SUCCESS - $count blocks produced
