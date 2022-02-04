#!/bin/bash
set -euo pipefail

. "${0%/*}/helpers/perform.sh"

echo '--- :anka: Pretest Setup'

if [[ ! $(python3 --version 2>/dev/null) ]]; then
   perform 'brew update'
   perform 'brew install python3'
fi

perform "./.cicd/test-package.run.sh"
