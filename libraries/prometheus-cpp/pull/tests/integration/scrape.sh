#!/usr/bin/env bash

telegraf=$(which telegraf)
if [ ! -x "$telegraf" ] ; then
    echo "telegraf must be in path for this test to run"
    exit 1
fi

function timeout_after
{
   ( echo failing after $1 seconds for execution && sleep $1 && kill $$ ) &
}

trap 'kill $(jobs -p)' EXIT

timeout_after 10

pull/tests/integration/sample-server &
telegraf --config pull/tests/integration/scrape.conf --quiet | grep -m1 http_requests_total
