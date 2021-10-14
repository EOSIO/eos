#!/bin/bash
set -eo pipefail
[[ "$RAW_PIPELINE_CONFIG" == '' ]] && export RAW_PIPELINE_CONFIG="$1"
[[ "$RAW_PIPELINE_CONFIG" == '' ]] && export RAW_PIPELINE_CONFIG='pipeline.jsonc'
[[ "$PIPELINE_CONFIG" == '' ]] && export PIPELINE_CONFIG='pipeline.json'
# read dependency file
if [[ -f "$RAW_PIPELINE_CONFIG" ]]; then
    echo 'Reading pipeline configuration file...'
    cat "$RAW_PIPELINE_CONFIG" | grep -Po '^[^"/]*("((?<=\\).|[^"])*"[^"/]*)*' | jq -c .\"eosio\" > "$PIPELINE_CONFIG"
    CDT_VERSION=$(cat "$PIPELINE_CONFIG" | jq -r '.dependencies."eosio.cdt"')
else
    echo 'ERROR: No pipeline configuration file or dependencies file found!'
    exit 1
fi

if [[ "$BUILDKITE" == 'true' ]]; then
    CDT_COMMIT=$((curl -s https://api.github.com/repos/EOSIO/eosio.cdt/git/refs/tags/$CDT_VERSION && curl -s https://api.github.com/repos/EOSIO/eosio.cdt/git/refs/heads/$CDT_VERSION) | jq '.object.sha' | sed "s/null//g" | sed "/^$/d" | tr -d '"' | sed -n '1p')
    test -z "$CDT_COMMIT" && CDT_COMMIT=$(echo $CDT_VERSION | tr -d '"' | tr -d "''" | cut -d ' ' -f 1) # if both searches returned nothing, the version is probably specified by commit hash already
fi

echo "Using cdt ${CDT_COMMIT} from \"$CDT_VERSION\"..."
export CDT_URL="https://eos-public-oss-binaries.s3-us-west-2.amazonaws.com/${CDT_COMMIT:0:7}-eosio.cdt-ubuntu-18.04_amd64.deb"

[[ "$ENABLE_INSTALL" == 'true' ]] || echo '--- :evergreen_tree: Configuring Environment'
. ./.cicd/helpers/general.sh
mkdir -p "$BUILD_DIR"
[[ -z "$DCMAKE_BUILD_TYPE" ]] && export DCMAKE_BUILD_TYPE='Release'
CMAKE_EXTRAS="-DCMAKE_BUILD_TYPE=\"$DCMAKE_BUILD_TYPE\" -DEOSIO_COMPILE_TEST_CONTRACTS=true -DENABLE_RODEOS=ON -DENABLE_TESTER=ON -DENABLE_MULTIVERSION_PROTOCOL_TEST=\"true\" -DAMQP_CONN_STR=\"amqp://guest:guest@localhost:5672\""


ARGS=${ARGS:-"--rm --init -v \"\$(pwd):$MOUNTED_DIR\""}
PRE_COMMANDS="cd \"$MOUNTED_DIR/build\""
# PRE_COMMANDS: Executed pre-cmake
# CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
[[ ! "$IMAGE_TAG" =~ 'unpinned' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DTPM2TSS_STATIC=\"On\" -DCMAKE_TOOLCHAIN_FILE=\"$MOUNTED_DIR/.cicd/helpers/clang.make\""
if [[ "$IMAGE_TAG" == 'ubuntu-18.04-unpinned' ]]; then
    CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER=\"clang++-7\" -DCMAKE_C_COMPILER=\"clang-7\" -DLLVM_DIR=\"/usr/lib/llvm-7/lib/cmake/llvm\""
fi

CDT_COMMAND="curl -sSf $CDT_URL --output eosio.cdt.deb && apt install ./eosio.cdt.deb"
CMAKE_COMMAND="cmake \$CMAKE_EXTRAS .."
MAKE_COMMAND="make -j $JOBS"
BUILD_COMMANDS="echo \"+++ :hammer_and_wrench: Building EOSIO\" && echo \"$CDT_COMMAND\" && eval $CDT_COMMAND && echo \"$ $CMAKE_COMMAND\" && eval $CMAKE_COMMAND && echo \"$ $MAKE_COMMAND\" && eval $MAKE_COMMAND"
# Docker Commands
if [[ "$BUILDKITE" == 'true' ]]; then
    # Generate Base Images
    BASE_IMAGE_COMMAND="\"$CICD_DIR/generate-base-images.sh\""
    echo "$ $BASE_IMAGE_COMMAND"
    eval $BASE_IMAGE_COMMAND
    [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="cp -r \"$MOUNTED_DIR\" \"/root/eosio\" && cd \"/root/eosio/build\" &&"
    COMMANDS="$COMMANDS $BUILD_COMMANDS"
    [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="$COMMANDS && make install"
elif [[ "$GITHUB_ACTIONS" == 'true' ]]; then
    ARGS="$ARGS -e JOBS"
    COMMANDS="$BUILD_COMMANDS"
else
    COMMANDS="$BUILD_COMMANDS"
fi

. "$HELPERS_DIR/file-hash.sh" "$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile"
COMMANDS="$PRE_COMMANDS && $COMMANDS"
DOCKER_RUN="docker run $ARGS $(buildkite-intrinsics) --env CMAKE_EXTRAS='$CMAKE_EXTRAS' '$FULL_TAG' bash -c '$COMMANDS'"
echo "$ $DOCKER_RUN"
eval $DOCKER_RUN

if [[ "$BUILDKITE" == 'true' && "$ENABLE_INSTALL" != 'true' ]]; then
    echo '--- :arrow_up: Uploading Artifacts'
    echo 'Compressing build directory.'
    tar -pczf 'build.tar.gz' build
    echo 'Uploading build directory.'
    buildkite-agent artifact upload 'build.tar.gz'
    echo 'Done uploading artifacts.'
fi
[[ "$ENABLE_INSTALL" == 'true' ]] || echo '--- :white_check_mark: Done!'
