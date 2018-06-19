from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr

import argparse

# pylint: disable=too-many-instance-attributes
class TestHelper(object):
    LOCAL_HOST="localhost"
    DEFAULT_PORT=8888

    @staticmethod
    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    def parse_args(includeArgs):
        """Accepts set of arguments, builds argument parser and returns parse_args() output."""
        assert(includeArgs)
        assert(isinstance(includeArgs, set))

        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('-?', action='help', default=argparse.SUPPRESS,
                                 help=argparse._('show this help message and exit'))

        if "-p" in includeArgs:
            parser.add_argument("-p", type=int, help="producing nodes count", default=1)
        if "-n" in includeArgs:
            parser.add_argument("-n", type=int, help="total nodes", default=0)
        if "-d" in includeArgs:
            parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
        if "--nodes-file" in includeArgs:
            parser.add_argument("--nodes-file", type=str, help="File containing nodes info in JSON format.")
        if "-s" in includeArgs:
            parser.add_argument("-s", type=str, help="topology", choices=["mesh"], default="mesh")
        if "-c" in includeArgs:
            parser.add_argument("-c", type=str, help="chain strategy",
                    choices=[Utils.SyncResyncTag, Utils.SyncNoneTag, Utils.SyncHardReplayTag],
                    default=Utils.SyncResyncTag)
        if "--kill-sig" in includeArgs:
            parser.add_argument("--kill-sig", type=str, choices=[Utils.SigKillTag, Utils.SigTermTag], help="kill signal.",
                    default=Utils.SigKillTag)
        if "--kill-count" in includeArgs:
            parser.add_argument("--kill-count", type=int, help="nodeos instances to kill", default=-1)
        if "--p2p-plugin" in includeArgs:
            parser.add_argument("--p2p-plugin", choices=["net", "bnet"], help="select a p2p plugin to use. Defaults to net.", default="net")
        if "--seed" in includeArgs:
            parser.add_argument("--seed", type=int, help="random seed", default=1)

        if "--host" in includeArgs:
            parser.add_argument("-h", "--host", type=str, help="%s host name" % (Utils.EosServerName),
                                     default=TestHelper.LOCAL_HOST)
        if "--port" in includeArgs:
            parser.add_argument("-p", "--port", type=int, help="%s host port" % Utils.EosServerName,
                                     default=TestHelper.DEFAULT_PORT)
        if "--prod-count" in includeArgs:
            parser.add_argument("-c", "--prod-count", type=int, help="Per node producer count", default=1)
        if "--defproducera_prvt_key" in includeArgs:
            parser.add_argument("--defproducera_prvt_key", type=str, help="defproducera private key.")
        if "--defproducerb_prvt_key" in includeArgs:
            parser.add_argument("--defproducerb_prvt_key", type=str, help="defproducerb private key.")
        if "--mongodb" in includeArgs:
            parser.add_argument("--mongodb", help="Configure a MongoDb instance", action='store_true')
        if "--dump-error-details" in includeArgs:
            parser.add_argument("--dump-error-details",
                                     help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                                     action='store_true')
        if "--dont-launch" in includeArgs:
            parser.add_argument("--dont-launch", help="Don't launch own node. Assume node is already running.",
                                     action='store_true')
        if "--keep-logs" in includeArgs:
            parser.add_argument("--keep-logs", help="Don't delete var/lib/node_* folders upon test completion",
                                     action='store_true')
        if "-v" in includeArgs:
            parser.add_argument("-v", help="verbose logging", action='store_true')
        if "--leave-running" in includeArgs:
            parser.add_argument("--leave-running", help="Leave cluster running after test finishes", action='store_true')
        if "--only-bios" in includeArgs:
            parser.add_argument("--only-bios", help="Limit testing to bios node.", action='store_true')
        if "--clean-run" in includeArgs:
            parser.add_argument("--clean-run", help="Kill all nodeos and kleos instances", action='store_true')
        if "--sanity-test" in includeArgs:
            parser.add_argument("--sanity-test", help="Validates nodeos and kleos are in path and can be started up.", action='store_true')

        args = parser.parse_args()
        return args

    @staticmethod
    # pylint: disable=too-many-arguments
    def shutdown(cluster, walletMgr, testSuccessful=True, killEosInstances=True, killWallet=True, keepLogs=False, cleanRun=True, dumpErrorDetails=False):
        """Cluster and WalletMgr shutdown and cleanup."""
        assert(cluster)
        assert(isinstance(cluster, Cluster))
        if walletMgr:
            assert(isinstance(walletMgr, WalletMgr))
        assert(isinstance(testSuccessful, bool))
        assert(isinstance(killEosInstances, bool))
        assert(isinstance(killWallet, bool))
        assert(isinstance(cleanRun, bool))
        assert(isinstance(dumpErrorDetails, bool))

        if testSuccessful:
            Utils.Print("Test succeeded.")
        else:
            Utils.Print("Test failed.")
        if not testSuccessful and dumpErrorDetails:
            cluster.dumpErrorDetails()
            if walletMgr:
                walletMgr.dumpErrorDetails()
            Utils.Print("== Errors see above ==")

        if killEosInstances:
            Utils.Print("Shut down the cluster.")
            cluster.killall(allInstances=cleanRun)
            if testSuccessful and not keepLogs:
                Utils.Print("Cleanup cluster data.")
                cluster.cleanup()

        if walletMgr and killWallet:
            Utils.Print("Shut down the wallet.")
            walletMgr.killall(allInstances=cleanRun)
            if testSuccessful and not keepLogs:
                Utils.Print("Cleanup wallet data.")
                walletMgr.cleanup()

