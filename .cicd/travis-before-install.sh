#!/bin/bash
set -e
# print information about host resources
echo '===== RESOURCES ====='
echo "$(getconf _NPROCESSORS_ONLN) CPU cores found."
if [[ "$(uname)" == Darwin ]]; then top -l 1 -s 0 | grep PhysMem; else vmstat -sSM | grep -i 'memory'; fi
df -h
echo '====================='