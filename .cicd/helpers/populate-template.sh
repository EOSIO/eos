#!/bin/bash
set -eo pipefail
# Collect commands from code block, add RUN before the start of commands, and add it to temporary template
DOC_CODE_BLOCK=$(cat docs/dep-install-${IMAGE_TAG:-$FILE_NAME}.md | sed -n '/```/,/```/p')
SANITIZED_COMMANDS=$(echo "$DOC_CODE_BLOCK" | grep -v -e '```' -e '\#.*' -e '^$' | sed "s/-b master/-b $BUILDKITE_BRANCH/g")
if [[ ! ${IMAGE_TAG:-$FILE_NAME} =~ 'macos' ]]; then # Linux / Docker
    DOCKER_COMMANDS=$(echo "$SANITIZED_COMMANDS" | awk '{if ( $0 ~ /^[ ].*/ ) { print $0 } else if ( $0 ~ /^PATH/ ) { print "ENV " $0 } else { print "RUN " $0 } }')
    FILE_EXTENSION=".dockerfile"
    APPEND_LINE=6
else # Mac OSX
    DOCKER_COMMANDS=$(echo "$SANITIZED_COMMANDS")
    FILE_EXTENSION=".sh"
    APPEND_LINE=4
fi
echo "$DOCKER_COMMANDS" > /tmp/docker-commands
awk "NR==$APPEND_LINE{print;system(\"cat /tmp/docker-commands\");next} 1" .cicd/platform-templates/${FILE:-"${IMAGE_TAG}$FILE_EXTENSION"}> /tmp/${FILE_NAME:-$IMAGE_TAG}
chmod +x /tmp/${FILE_NAME:-$IMAGE_TAG}