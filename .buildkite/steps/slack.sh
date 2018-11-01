#!/bin/bash
set -euo pipefail

SLACK_MESSAGE=${1:-"No message"}
SLACK_COLOR=${2:-"bad"}

curl -X POST -H 'Content-type: application/json' \
--data "{\"text\":\"${SLACK_MESSAGE}\", \
    \"attachments\":[ \
        { \
             \"fallback\":\"${SLACK_MESSAGE}\", \
             \"color\":\"${SLACK_COLOR}\", \
             \"fields\":[ \
                { \
                   \"title\":\"Branch\", \
                   \"value\":\"$BUILDKITE_BRANCH\", \
                   \"short\":false \
                }, \
                { \
                   \"title\":\"Creator\", \
                   \"value\":\"$BUILDKITE_BUILD_CREATOR\", \
                   \"short\":false \
                }, \
                { \
                   \"title\":\"URL\", \
                   \"value\":\"<$BUILDKITE_BUILD_URL|$BUILDKITE_BUILD_URL>\", \
                   \"short\":false \
                } \
             ] \
        } \
    ] \
}" ${WEBHOOK}
