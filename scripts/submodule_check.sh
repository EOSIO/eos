#!/bin/bash

REPO_DIR=`mktemp -d`
git clone "$BUILDKITE_REPO" "$REPO_DIR"
git submodule update --init --recursive
cd "$REPO_DIR"

declare -A PR_MAP
declare -A BASE_MAP

echo "getting submodule info for $BUILDKITE_BRANCH"
git checkout "$BUILDKITE_BRANCH" &> /dev/null
git submodule update --init &> /dev/null
while read -r a b; do
  PR_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')

echo "getting submodule info for $BUILDKITE_PULL_REQUEST_BASE_BRANCH"
git checkout "$BUILDKITE_PULL_REQUEST_BASE_BRANCH" &> /dev/null
git submodule update --init &> /dev/null
while read -r a b; do
  BASE_MAP[$a]=$b
done < <(git submodule --quiet foreach --recursive 'echo $path `git log -1 --format=%ct`')

for k in "${!BASE_MAP[@]}"; do
  base_ts=${BASE_MAP[$k]}
  pr_ts=${PR_MAP[$k]}
  echo "submodule $k"
  echo "  timestamp on $BUILDKITE_BRANCH: $pr_ts"
  echo "  timestamp on $BUILDKITE_PULL_REQUEST_BASE_BRANCH: $base_ts"
  if (( $pr_ts < $base_ts)); then
    echo "$k is older on $BUILDKITE_BRANCH than $BUILDKITE_PULL_REQUEST_BASE_BRANCH; investigating..."

    if for c in `git log $BUILDKITE_BRANCH ^$BUILDKITE_PULL_REQUEST_BASE_BRANCH --pretty=format:"%H"`; do git show --pretty="" --name-only $c; done | grep -q "^$k$"; then
      echo "ERROR: $k has regressed"
      exit 1
    else
      echo "$k was not in the diff; no regression detected"
    fi
  fi
done
