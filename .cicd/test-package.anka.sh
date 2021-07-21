#!/bin/bash
set -euo pipefail

. "${0%/*}/libfunctions.sh"

echo '--- :anka: Pretest Setup'

if [[ ! $(brew --version 2>/dev/null) ]]; then
   perform 'echo | /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"'
   perform 'brew analytics off'
fi

if [[ ! $(python3 --version 2>/dev/null) ]]; then
   perform 'brew update'
   perform 'brew install python3'
fi

perform "./.cicd/test-package.run.sh"
