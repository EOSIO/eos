#!/usr/bin/env python3

import testUtils

import decimal
import re

###############################################################
# nodeos_voting_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################

Print=testUtils.Utils.Print
errorExit=testUtils.Utils.errorExit
cmdError=testUtils.Utils.cmdError

from core_symbol import CORE_SYMBOL

# create TestState with all common flags, except --mongodb
testState=testUtils.TestState("--mongodb")

args = testState.parse_args()
testState.cluster=testUtils.Cluster(walletd=True)
testState.pnodes=4
testState.totalNodes=4
testState.totalProducers=testState.pnodes*21


try:
    testState.start(testUtils.Cluster(walletd=True))

    testState.success()
finally:
    testState.end()

exit(0)
