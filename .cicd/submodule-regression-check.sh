#!/bin/bash
set -eo pipefail
declare -A PR_MAP
declare -A BASE_MAP

if [[ $BUILDKITE == true ]]; then
    [[ -z $BUILDKITE_PULL_REQUEST_BASE_BRANCH ]] && echo "Unable to find BUILDKITE_PULL_REQUEST_BASE_BRANCH ENV. Skipping submodule regression check." && exit 0
    BASE_BRANCH="$(echo "$BUILDKITE_PULL_REQUEST_BASE_BRANCH" | sed 's.^/..')"
    CURRENT_BRANCH="$(echo "$BUILDKITE_BRANCH" | sed 's.^/..')"
else
    [[ -z $GITHUB_BASE_REF ]] && echo "Cannot find \$GITHUB_BASE_REF, so we have nothing to compare submodules to. Skipping submodule regression check." && exit 0
    BASE_BRANCH=$GITHUB_BASE_REF
    CURRENT_BRANCH="refs/remotes/pull/$PR_NUMBER/merge"
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

echo "switching back to $CURRENT_BRANCH..."
echo "git checkout -qf $CURRENT_BRANCH"
git checkout -qf $CURRENT_BRANCH 1> /dev/null

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