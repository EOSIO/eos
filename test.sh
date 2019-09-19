#!/bin/bash
set -eo pipefail

COMMANDS="ls -laht / && echo 123"

echo $COMMANDS
bash -c "$COMMANDS"