#!/bin/bash
. /tmp/.helpers
$PRE_COMMANDS
fold-execute ccache -s
mkdir /workdir/build
cd /workdir/build
fold-execute cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true $CMAKE_EXTRAS /workdir
fold-execute make -j$JOBS
[[ "${ENABLE_PARALLEL_TESTS:-true}" != 'false' ]] && fold-execute ctest -j $JOBS -LE _tests --output-on-failure -T Test
if ${ENABLE_SERIAL_TESTS:-true}; then
    mkdir mongodb
    fold-execute mongod --dbpath ./mongodb --fork --logpath mongod.log
    fold-execute ctest -L nonparallelizable_tests --output-on-failure -T Test
fi
[[ "${ENABLE_LR_TESTS:-false}" != 'false' ]] && fold-execute ctest -L long_running_tests --output-on-failure -T Test
if ! ${TRAVIS:-false}; then
    cd ..
    [[ $(pgrep mongod) ]] && pgrep mongod | xargs kill -9
    tar -pczf build.tar.gz build
    buildkite-agent artifact upload build.tar.gz --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
fi