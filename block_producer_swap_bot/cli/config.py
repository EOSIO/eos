"""
Provide implementation of the CLI configurations file.
"""
import configparser

from cli.constants import CONFIG_FILE_NAME


class ConfigFile:
    """
    Implementation of command line interface configuration file.
    """

    def __init__(self, name=CONFIG_FILE_NAME):
        """
        Parse configuration file.
        """
        self.config = configparser.ConfigParser()
        self.config.read(name)

    def get_remnode(self):
        """
        Get remnode from config file.
        """
        return self.config.get('NODES', 'remnode')

    def get_eth_provider(self):
        """
        Get ethereum provider from config file.
        """
        return self.config.get('NODES', 'eth-provider')

    def get_block_producer_permission(self):
        """
        Get block producer permission to sign process-swap transactions.
        """
        return self.config.get('REM', 'block-producer-permission')

    def get_block_producer_private_key(self):
        """
        Get block producer private key to sign process-swap transactions.
        """
        return self.config.get('REM', 'block-producer-private-key')
