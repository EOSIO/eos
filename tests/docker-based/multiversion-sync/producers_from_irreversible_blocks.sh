#!/bin/bash

host=$1
endpoint=http://$host:8888

function get_block_producer {
   block_num=$1
   curl -fs -X POST -H "Content-Type: application/json" -d '{"block_num_or_id": '$block_num'}' $endpoint/v1/chain/get_block | jq -r ".producer"
}

function block_num {
  local block_type=$1
  curl -fs $endpoint/v1/chain/get_info | jq ".${block_type}_block_num"
}

lib_num=$(block_num last_irreversible)

declare -A producerSet

while [[ $lib_num > 1 ]]; do
   producer=$(get_block_producer $lib_num)
   if [[ -v "producerSet[$producer]" ]];then 
      break
   else
      producerSet[$producer]==1
   fi
   ((lib_num-=12))
done

echo "${!producerSet[@]}"