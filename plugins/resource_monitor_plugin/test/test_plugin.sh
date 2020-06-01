#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

LOG_FILE="/tmp/resource_monitor.log"
DATA_DIR="/tmp/data"
BLOCK_DIR=$DATA_DIR/blocks
STATE_DIR=$DATA_DIR/state

CMD="../../../build/bin/nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin -d $DATA_DIR"

num_tests=1

# Arguments $1: test name, $2: additional command line,
# $3: the string which must appear in the nodeos log file before
# terminating nodeos, the remaining arguments must appear in
# the log file for the test to be considered success.
run_test() {
  printf "Test $num_tests: $1  ...... "
  rm -f $LOG_FILE

  mkdir -p $BLOCK_DIR $STATE_DIR
  rm -fr $BLOCK_DIR/* $STATE_DIR/*

  local my_cmd="$CMD $2 > $LOG_FILE 2>&1 &"
  eval $my_cmd


  while [ ! -f "$LOG_FILE" ]; do
    sleep 1
  done

  sleep_secs=0
  while [ $sleep_secs -lt 60 ]; do
    if grep -Fq "$3" $LOG_FILE
    then
      break
    fi

    sleep 1
    ((sleep_secs=sleep_secs+1))
  done

  if [ "$3" == "Produced block" ]; then
    pkill nodeos
  fi

  for ((i = 4; i <= $#; i++ )); do
    if grep -Fq "${!i}" $LOG_FILE
    then
      printf "${GREEN}Success${NC}\n"
    else
      printf "${RED}failed${NC}\n"
      exit
    fi
  done

  ((num_tests=num_tests+1))
}

######### Tests #########

run_test "Interval too big" "--plugin eosio::resource_monitor_plugin --resource-monitor-interval-seconds=60" "plugin_config_exception" "must be greater than 0 and less than 60"

run_test "Interval too small" "--plugin eosio::resource_monitor_plugin --resource-monitor-interval-seconds=0" "plugin_config_exception" "must be greater than 0 and less than 60"

run_test "Interval at lower boundary" "--plugin eosio::resource_monitor_plugin --resource-monitor-interval-seconds=1 --resource-monitor-space-threshold=2" "nodeos successfully exiting" "sleep_time = 1"

run_test "Interval at upper boundary" "--plugin eosio::resource_monitor_plugin --resource-monitor-interval-seconds=59 --resource-monitor-space-threshold=2" "nodeos successfully exiting" "sleep_time = 59"

run_test "Interval not supplied" "--plugin eosio::resource_monitor_plugin --resource-monitor-space-threshold=2" "nodeos successfully exiting" "sleep_time = 2"

run_test "Threshold too big" "--plugin eosio::resource_monitor_plugin --resource-monitor-space-threshold=100" "plugin_config_exception" "must be greater than 0 and less than 100"

run_test "Threshold too small" "--plugin eosio::resource_monitor_plugin --resource-monitor-space-threshold=0" "plugin_config_exception" "must be greater than 0 and less than 100"

run_test "Threshold at lower boundary" "--plugin eosio::resource_monitor_plugin --resource-monitor-space-threshold=1" "nodeos successfully exiting" "file system exceeded configured threshold 1%"

run_test "Threshold at upper boundary" "--plugin eosio::resource_monitor_plugin --resource-monitor-space-threshold=99" "Produced block" "threshold = 99" "sleep_time = 2"

run_test "Threshold or Interval not supplied" "--plugin eosio::resource_monitor_plugin" "Produced block" "threshold = 90" "sleep_time = 2"

run_test "TraceApi plugin without Resource Monitor" "--plugin eosio::trace_api_plugin --trace-dir=/tmp/trace --trace-no-abis" "Produced block" "/tmp/trace registered to be monitored"

run_test "TraceApi plugin with Resource Monitor first" " --plugin eosio::resource_monitor_plugin --plugin eosio::trace_api_plugin --trace-dir=/tmp/trace --trace-no-abis" "Produced block" "/tmp/trace's file system already monitored"

run_test "TraceApi plugin with Resource Monitor last" "--plugin eosio::trace_api_plugin --trace-dir=/tmp/trace --trace-no-abis --plugin eosio::resource_monitor_plugin" "Produced block" "/tmp/trace's file system already monitored"

run_test "Producer plugin without Resource Monitor" " " "Produced block" "/tmp/data/snapshots registered to be monitored"

run_test "Producer plugin with Resource Monitor" "--plugin eosio::resource_monitor_plugin" "Produced block" "/tmp/data/snapshots's file system already monitored"

run_test "Chain plugin without Resource Monitor" " " "Produced block" "/tmp/data/blocks registered to be monitored" "protocol_features registered to be monitored"

run_test "Chain plugin with Resource Monitor" "--plugin eosio::resource_monitor_plugin" "Produced block" "blocks's file system already monitored" "protocol_features's file system already monitored"

run_test "State History plugin without Resource Monitor" "--plugin eosio::state_history_plugin --state-history-dir=/tmp/state-history --disable-replay-opts" "Produced block" "/tmp/state-history registered to be monitored"

run_test "State History plugin with Resource Monitor first" "--plugin eosio::resource_monitor_plugin --plugin eosio::state_history_plugin --state-history-dir=/tmp/state-history --disable-replay-opts" "Produced block" "/tmp/state-history's file system already monitored"

run_test "State History plugin with Resource Monitor last" "--plugin eosio::state_history_plugin --state-history-dir=/tmp/state-history --disable-replay-opts --plugin eosio::resource_monitor_plugin" "Produced block" "/tmp/state-history's file system already monitored"

run_test "State History and Trace Api  with Resource Monitor" "--plugin eosio::state_history_plugin --state-history-dir=/tmp/state-history --disable-replay-opts --plugin eosio::trace_api_plugin --trace-dir=/tmp/trace --trace-no-abis  --plugin eosio::resource_monitor_plugin" "Produced block" "/tmp/state-history's file system already monitored" "/tmp/trace's file system already monitored"
