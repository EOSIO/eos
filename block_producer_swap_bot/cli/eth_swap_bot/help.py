"""
Provide help messages for command line interface's eth_swap_bot commands.
"""
PERMISSION_ARGUMENT_HELP_MESSAGE = 'A block producer account and permission' \
                                   ' of level to authorize, as in \'account@permission\''

TXID_ARGUMENT_HELP_MESSAGE = 'Identifier of the transaction with swap request on Ethereum'
CHAIN_ID_ARGUMENT_HELP_MESSAGE = 'Identifier of destination blockchain on which swapped tokens should be sent'
SWAP_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE = 'Public key which is used to validate signature for' \
                                        'claiming account name on Remchain'
ETH_TIMESTAMP_ARGUMENT_HELP_MESSAGE = 'Ethereum timestamp of swap request transaction in format yyyy-mm-dd'
