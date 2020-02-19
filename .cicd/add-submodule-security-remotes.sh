#!/bin/bash
set -eo pipefail
# Add repos to the below MAP if their security repo name differs
## Format: SUBMODULE_NAME[space]SECURITY_SUBMODULE_NAME
export MAP=$(cat <<-'LIST'
    eos eosio-security
LIST
)
export count=0
gather-and-add() {
    ((count+=1))
    [ $count -eq 100 ] && echo "100 levels of submodules reached. This is likely a problem, so we're going to stop." && exit 50
    SUBMODULES=$(git submodule)
    oIFS=$IFS
    IFS=$'\n'
    echo "$SUBMODULES"
    for SUBMOD in $SUBMODULES; do
        SUBMOD_COMMIT=$(echo $SUBMOD | awk '{print $1}' | sed 's/+//g' | sed 's/-//g')
        SUBMOD_PATH=$(echo $SUBMOD | awk '{print $2}')
        FULL_SUBMOD_PATH="$1/$SUBMOD_PATH"
        SUBMOD_NAME=$(basename $SUBMOD_PATH)
        SEC_SUBMOD_NAME="${SUBMOD_NAME}-security"
        for ITEM in $MAP; do
            ITEM_SUBMODULE=$(echo $ITEM | awk '{print $1}')
            ITEM_SUBMODULE_ACTUAL=$(echo $ITEM | awk '{print $2}')
            if [[ $ITEM_SUBMODULE == $SUBMOD_NAME ]]; then
                echo " - Found $ITEM_SUBMODULE, but its security repo is named $ITEM_SUBMODULE_ACTUAL. Using that instead!"
                SEC_SUBMOD_NAME=$ITEM_SUBMODULE_ACTUAL
            fi
        done
        ORIGINAL_REMOTE_URL="git@github.com:$([[ -f .gitmodules ]] && cat .gitmodules | grep 'url =' | grep $SUBMOD_NAME | awk -F'.com' '{print $2}' || echo EOSIO/$SUBMOD_NAME)"
        REMOTE_URL="git@github.com:EOSIO/$SEC_SUBMOD_NAME.git"
        echo "Preparing $SUBMOD_NAME [$FULL_SUBMOD_PATH] with security remote $REMOTE_URL"
        git clone -n $ORIGINAL_REMOTE_URL $FULL_SUBMOD_PATH || true
        cd $FULL_SUBMOD_PATH
        ADD_RETURN=$(git remote add remote $REMOTE_URL 2>&1 || true)
        ( [[ ! "$ADD_RETURN" =~ "fatal: remote remote already exists." ]] && [[ ! -z "$ADD_RETURN" ]] ) && printf "Problem with adding the remote!\n$ADD_RETURN" && exit 1
        PULL_RETURN=$(git pull remote 2>&1 || true)
        ( [[ ! "$PULL_RETURN" =~ "ERROR: Repository not found." ]] && [[ ! "$PULL_RETURN" =~ "You are not currently on a branch" ]] && [[ ! "$PULL_RETURN" =~ "You asked to pull from the remote" ]] ) && printf "Something is wrong with pulling the remote branches!\n$PULL_RETURN" && exit 1 # Don't let it proceed if it's NOT either A: The -security branch doesn't exist OR B: The pull was successful. A networking issue could cause a checkout to fail and then the whole build to throw errors that could waste engineer troubleshooting time.
        cd $1 && git submodule update --init --force -- $SUBMOD_PATH
        cd $FULL_SUBMOD_PATH
        if [[ ! -z "$(ls .)" ]] && [[ ! -z $(git submodule) ]]; then
            gather-and-add "$FULL_SUBMOD_PATH" # Make sure to add a security remote for sub-submodules; only if the folder isn't empty and it has sub-submodules
        fi
    done
    IFS=$oIFS
}
[[ $BUILDKITE_LABEL == ':pipeline: Pipeline Upload' ]] && exit 0 # Don't do anything if the step is Pipeline Upload
echo "[Preparing Submodules]"
if [[ $BUILDKITE_PIPELINE_SLUG =~ '-security' ]] || [[ ${FORCE_SUBMODULE_REMOTE_PREP:-false} == true ]]; then
    gather-and-add "$(pwd)"
else
    echo "Skipped remote url addition for $BUILDKITE_PIPELINE_SLUG..."
fi
git submodule update --init --recursive --force # ensure we obtain submodule submodules