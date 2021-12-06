#!/bin/bash
set -eo pipefail
echo '+++ :ladybug: Buildfleet Debugging'
echo 'We are hanging Buildkite agents to debug the "Agent Lost" error.'
printf "See \033]1339;url=https://blockone.atlassian.net/browse/BLU-27876;content=BLU-27876\a for more information.\n"
SLEEP_COMMAND="sleep ${$SLEEP:-60}"
echo "$ $SLEEP_COMMAND"
eval $SLEEP_COMMAND
echo '--- :white_check_mark: Done!'
