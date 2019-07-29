#!/usr/bin/env bash
set -eo pipefail
echo '+++ :gear: Configuring Travis CI Build'
[[ -z $TRIGGER_TRAVIS_API_KEY ]] && (echo '+++ :no_entry: ERROR: Travis CI API key not found!' && exit 1)
TRAVIS_MESSAGE="$BUILDKITE_MESSAGE - $(echo $BUILDKITE_BUILD_URL | grep -oe 'buildkite.*')"
[[ -z $BODY ]] && BODY="$(jq -nc '{"request":{"branch":env.BUILDKITE_BRANCH,"message":env.TRAVIS_MESSAGE}}')"
echo 'Configuration:'
echo "$BODY" | jq
echo 'This configuration is derived from the BUILDKITE_BRANCH and BUILDKITE_MESSAGE variables set for this build.'
echo '+++ :tractor: Triggering Travis CI Build'
export RESULT="$(curl -s -X POST -H "Content-Type: application/json" -H "Accept: application/json" -H "Travis-API-Version: 3" -H "Authorization: token $TRIGGER_TRAVIS_API_KEY" -d "$BODY" https://api.travis-ci.org/repo/EOSIO%2feosio.contracts/requests)"
echo 'API Response:'
echo "$RESULT" | jq 2>/dev/null || echo "$RESULT"
if [[ "$(echo $RESULT | jq -r '.["@type"]')" != "pending" ]]; then
    echo '+++ :no_entry: ERROR: Failed to trigger Travis API!'
    exit 1
fi