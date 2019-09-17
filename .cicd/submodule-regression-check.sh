#!/bin/bash
set -eo pipefail
declare -A PR_MAP
declare -A BASE_MAP
# Support Travis and BK
if ${TRAVIS:-false}; then
    BASE_BRANCH=$TRAVIS_BRANCH
    CURRENT_BRANCH=${TRAVIS_PULL_REQUEST_BRANCH:-$TRAVIS_BRANCH} # We default to TRAVIS_BRANCH if it's not a PR so it passes on non PR runs
else
    BASE_BRANCH=${BUILDKITE_PULL_REQUEST_BASE_BRANCH:-$BUILDKITE_BRANCH}
    CURRENT_BRANCH=$BUILDKITE_BRANCH
fi
echo "getting submodule info for $CURRENT_BRANCH"
while read -r a b; do
    PR_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')
echo "getting submodule info for $BASE_BRANCH"
git checkout $BASE_BRANCH &> /dev/null
git submodule update --init &> /dev/null
while read -r a b; do
    BASE_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')
for k in "${!BASE_MAP[@]}"; do
    base_ts=${BASE_MAP[$k]}
    pr_ts=${PR_MAP[$k]}
    echo "submodule $k"
    echo "  timestamp on $CURRENT_BRANCH: $pr_ts"
    echo "  timestamp on $BASE_BRANCH: $base_ts"
    if (( $pr_ts < $base_ts)); then
        echo "$k is older on $CURRENT_BRANCH than $BASE_BRANCH; investigating..."
        if for c in `git log $CURRENT_BRANCH ^$BASE_BRANCH --pretty=format:"%H"`; do git show --pretty="" --name-only $c; done | grep -q "^$k$"; then
            echo "ERROR: $k has regressed"
            exit 1
        else
            echo "$k was not in the diff; no regression detected"
        fi
    fi
done