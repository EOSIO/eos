#!/bin/bash
set -eo pipefail
[[ -f build.tar.gz ]] && tar -xzf build.tar.gz
cd ./build
echo "[Killing old MongoDB]"
$(pgrep mongod | xargs kill -9) || true
echo "[Starting MongoDB]"
PATH=$PATH:~/opt/mongodb/bin
mongod --fork --dbpath ~/data/mongodb -f ~/etc/mongod.conf --logpath ~/var/log/mongodb/mongod.log
echo "[Running tests]"
set +e # defer ctest error handling to end
ctest -L nonparallelizable_tests --output-on-failure -T Test
EXIT_STATUS=$?
[[ $EXIT_STATUS == 0 ]] && set -e
echo "[Uploading artifacts]"
mv ./Testing/$(ls ./Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml
buildkite-agent artifact upload test-results.xml
mv ~/var/log/mongodb/mongod.log mongod.log
buildkite-agent artifact upload mongod.log
# Throw proper exit code
[[ $EXIT_STATUS != 0 ]] && echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"; exit $EXIT_STATUS