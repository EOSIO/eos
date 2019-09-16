#!/bin/bash
set -eo pipefail
[[ $(uname) == 'Darwin' ]] && echo "Submodule regression doesn't run on mac (declare: -A: invalid option).." && exit 0
declare -A PR_MAP
declare -A BASE_MAP
# Support Travis and BK
if ${TRAVIS:-false}; then
    BASE_BRANCH=$TRAVIS_BRANCH
    CURRENT_BRANCH=${TRAVIS_PULL_REQUEST_BRANCH:-$TRAVIS_BRANCH} # We default to TRAVIS_BRANCH if it's not a PR so it passes on non PR runs
    [[ ! -z $TRAVIS_PULL_REQUEST_SLUG ]] && CURRENT_BRANCH=$TRAVIS_COMMIT # Support git log & echo output
else
    BASE_BRANCH=${BUILDKITE_PULL_REQUEST_BASE_BRANCH:-$BUILDKITE_BRANCH}
    CURRENT_BRANCH=$BUILDKITE_BRANCH
fi
[[ $BASE_BRANCH == $CURRENT_BRANCH ]] && echo 'BASE_BRANCH and CURRENT_BRANCH are the same' && exit 0

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
        if [[ $TRAVIS == true && ! -z $TRAVIS_PULL_REQUEST_SLUG ]]; then # IF it's a forked PR, we need to switch back to the PR ref/head so we can git log properly
            echo "git fetch origin +refs/pull/$TRAVIS_PULL_REQUEST/merge:"
            git fetch origin +refs/pull/$TRAVIS_PULL_REQUEST/merge: &> /dev/null
            echo "switching back to $TRAVIS_PULL_REQUEST_SLUG:$TRAVIS_PULL_REQUEST_BRANCH ($TRAVIS_COMMIT)"
            echo 'git checkout -qf FETCH_HEAD'
            git checkout -qf FETCH_HEAD &> /dev/null
        elif [[ $BUILDKITE == true ]]; then
            echo "switching back to $CURRENT_BRANCH"
            git checkout -f $CURRENT_BRANCH &> /dev/null
        fi
        if [[ ! -z $(for c in $(git --no-pager log $CURRENT_BRANCH ^$BASE_BRANCH --pretty=format:"%H"); do git show --pretty="" --name-only $c; done | grep "^$k$") ]]; then
            echo "ERROR: $k has regressed"
            exit 1
        else
            echo "$k was not in the diff; no regression detected"
        fi
    fi
done