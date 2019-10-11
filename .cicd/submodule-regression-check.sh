#!/bin/bash
set -eo pipefail
declare -A PR_MAP
declare -A BASE_MAP
# Support Travis and BK
if ${TRAVIS:-false}; then
    [[ -z $TRAVIS_PULL_REQUEST_BRANCH ]] && echo "Unable to find TRAVIS_PULL_REQUEST_BRANCH ENV. Skipping submodule regression check." && exit 0
    BASE_BRANCH=$TRAVIS_BRANCH
    CURRENT_BRANCH=$TRAVIS_PULL_REQUEST_BRANCH
    [[ ! -z $TRAVIS_PULL_REQUEST_SLUG ]] && CURRENT_BRANCH=$TRAVIS_COMMIT # When we're not running from a PR, the slug is not set. When we are, we need to use the TRAVIS_COMMIT to be sure we're supporting the Forked PR's merge/code that's in the EOS repo. This is needed for the git log below.
else
    [[ -z $BUILDKITE_PULL_REQUEST_BASE_BRANCH ]] && echo "Unable to find BUILDKITE_PULL_REQUEST_BASE_BRANCH ENV. Skipping submodule regression check." && exit 0
    BASE_BRANCH=$BUILDKITE_PULL_REQUEST_BASE_BRANCH
    CURRENT_BRANCH=$BUILDKITE_BRANCH
fi

echo "getting submodule info for $CURRENT_BRANCH"
while read -r a b; do
    PR_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')

echo "getting submodule info for $BASE_BRANCH"
git checkout $BASE_BRANCH 1> /dev/null
git submodule update --init 1> /dev/null
while read -r a b; do
    BASE_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')

# We need to switch back to the PR ref/head so we can git log properly
if [[ $TRAVIS == true && ! -z $TRAVIS_PULL_REQUEST_SLUG ]]; then
    echo "git fetch origin +refs/pull/$TRAVIS_PULL_REQUEST/merge:"
    git fetch origin +refs/pull/$TRAVIS_PULL_REQUEST/merge: 1> /dev/null
    echo "switching back to $TRAVIS_COMMIT"
    echo 'git checkout -qf FETCH_HEAD'
    git checkout -qf FETCH_HEAD 1> /dev/null
elif [[ $BUILDKITE == true ]]; then
    echo "switching back to $CURRENT_BRANCH"
    git checkout -f $CURRENT_BRANCH 1> /dev/null
fi

for k in "${!BASE_MAP[@]}"; do
    base_ts=${BASE_MAP[$k]}
    pr_ts=${PR_MAP[$k]}
    echo "submodule $k"
    echo "  timestamp on $CURRENT_BRANCH: $pr_ts"
    echo "  timestamp on $BASE_BRANCH: $base_ts"
    if (( $pr_ts < $base_ts)); then
        echo "$k is older on $CURRENT_BRANCH than $BASE_BRANCH; investigating the difference between $CURRENT_BRANCH and $BASE_BRANCH to look for $k changing..."
        if [[ ! -z $(for c in $(git --no-pager log $CURRENT_BRANCH ^$BASE_BRANCH --pretty=format:"%H"); do git show --pretty="" --name-only $c; done | grep "^$k$") ]]; then
            echo "ERROR: $k has regressed"
            exit 1
        else
            echo "$k was not in the diff; no regression detected"
        fi
    fi
done