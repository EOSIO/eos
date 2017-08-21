#!/bin/bash

for a in  tn_data_*; do kill `cat $a/eosd.pid`; done
