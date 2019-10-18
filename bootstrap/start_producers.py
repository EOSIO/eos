from testnet_bootstrap import producers
from start_bootnode import start_node

producers_quantity = 3


def start_producers():
    for i in range(1, producers_quantity+1):
        start_node(i, producers['producers'][i-1])


if __name__ == '__main__':
    start_producers()
