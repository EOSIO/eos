"""
Provide help messages for command line interface's eth_swap_contract commands.
"""
ETH_PROVIDER_ARGUMENT_HELP_MESSAGE = 'Ethereum provider to pull swap requests from.'

TXID_ARGUMENT_HELP_MESSAGE = 'Identifier of the transaction with swap request.'
CHAIN_ID_ARGUMENT_HELP_MESSAGE = 'Identifier of destination blockchain on which swapped tokens should be sent.'
USER_PUBLIC_KEY_ARGUMENT_HELP_MESSAGE = 'Public key which is used to validate signature for' \
                                        'claiming account name on Remchain.'
AMOUNT_TO_SWAP_ARGUMENT_HELP_MESSAGE = 'Amount of tokens to swap from ERC20 REM to Remchain.'
TIMESTAMP_ARGUMENT_HELP_MESSAGE = 'Ethereum timestamp of request swap transaction.'
