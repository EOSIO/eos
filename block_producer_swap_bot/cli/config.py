"""
Provide implementation of the CLI configurations file.
"""
from cli.constants import CONFIG_FILE_NAME
import configparser


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

    def get_nodeos_url(self):
        return self.config.get('NODES', 'nodeos-url')

    def get_eth_provider(self):
        return self.config.get('NODES', 'eth-provider')

    def get_block_producer_permission(self):
        return self.config.get('EOSIO', 'block-producer-permission')
