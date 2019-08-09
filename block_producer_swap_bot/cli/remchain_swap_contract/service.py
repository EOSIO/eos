"""
Provide implementation of the remchain_swap_contract.
"""
from cli.constants import (
    INIT_SWAP_ACTION,
    REM_SWAP_ACCOUNT,
)

from eosiopy.eosioparams import EosioParams
from eosiopy.nodenetwork import NodeNetwork
from eosiopy.rawinputparams import RawinputParams
from eosiopy import eosio_config


class RemchainSwapContract:
    """
    Implements remchain_swap_contract.
    """

    def __init__(self, remnode, permission, private_key):
        """
        Constructor.

        Arguments:
            remnode (string, required): remnode script path with options
            permission (string, required): a permission to sign process swap transactions
            private_key (string, required): a block producer's private key
        """
        self.remnode = remnode
        self.permission = permission
        self.private_key = private_key

    def init(self, **kwargs):
        """
        Initialize swap on Remchain.

        Arguments:
            rampayer (string, required): sender of initialize swap action
            txid (string, required): an identifier of swap request transaction on the source blockchain
            swap_pubkey (string, required): a public key to verify signature with receiver and active, owner public keys
            amount (int, required): amount of tokens to swap
            return_address (string, required): return address on which tokens should be sent in case of cancellation
            return_chain_id (string, required): blockchain identifier on which tokens should be sent in case of cancellation
            timestamp (string, required): timestamp in UTC format of request swap transaction
                                          on the source blockchain (yyyy-mm-ddThh:mm:ss)

        """
        rampayer = self.permission.split('@')[0]
        txid = kwargs.get('txid')
        swap_pubkey = kwargs.get('swap_pubkey')
        return_address = kwargs.get('return_address')
        return_chain_id = kwargs.get('return_chain_id')
        amount = kwargs.get('amount')
        timestamp = kwargs.get('timestamp')

        raw = RawinputParams(INIT_SWAP_ACTION, {
                "rampayer": rampayer,
                "txid": txid,
                "swap_pubkey": swap_pubkey,
                "amount": amount,
                "return_address": return_address,
                "return_chain_id": return_chain_id,
                "timestamp": timestamp,
            }, REM_SWAP_ACCOUNT, self.permission)
        eosiop_arams = EosioParams(raw.params_actions_list, self.private_key)
        net = NodeNetwork.push_transaction(eosiop_arams.trx_json)
        print(net)
