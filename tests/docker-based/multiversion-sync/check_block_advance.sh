#!/bin/bash
set -ex

#
# Usage: check_block_advaince.sh block_type sleep_interval host1 ...
#    
#   block_type: last_irreversible or head
#   interval:   sleep time in seconds 
#


block_type=$1
interval=$2
shift; shift;
hosts=( "$@" )

if [[ ${#hosts[@]} == 0 ]]; then
   hosts=("localhost")
fi

echo "$block_type $interval ${hosts[@]}"

function block_num {
  local block_type=$1
  local host=$2

  curl -fs http://$host:8888/v1/chain/get_info | jq -c ".${block_type}_block_num"
}

declare -A initial_blocks_nums

for host in "${hosts[@]}"; do
   initial_blocks_nums[$host]=$(block_num $block_type $host)
done

sleep $interval

num_stalled_hosts=0

for host in "${hosts[@]}"; do
   if [ $(block_num $block_type $host) -le ${initial_blocks_nums[$host]} ]; then
      >&2 echo "$host $block_type block stalls at block ${initial_blocks_nums[$host]}"
      ((num_stalled_hosts++))
   fi
done

[[ ${num_stalled_hosts} == 0 ]]