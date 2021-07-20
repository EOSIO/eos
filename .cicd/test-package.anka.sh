#!/bin/bash
set -euo pipefail

function perform {
    echo "$ $1"
    eval $1
}

if [[ ! $(brew --version 2>/dev/null) ]]; then # macos
   perform 'echo | /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"'
   perform 'brew analytics off'
fi

./.cicd/test-package.run.sh
