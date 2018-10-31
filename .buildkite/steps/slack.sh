#!/bin/bash
set -euo pipefail

SLACK_MESSAGE=${1:-"No message"}

curl -X POST -H 'Content-type: application/json' \
--data "{\"text\":\"${SLACK_MESSAGE}\", \
    \"attachments\":[ \
        { \
             \"fallback\":\"${SLACK_MESSAGE}\", \
             \"color\":\"good\", \
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
