import subprocess

class P2PTestPeers:
    Debug = False

    hosts = ["localhost", "127.0.0.1"]
    ports = [8888, 8888]

    @staticmethod
    def exec(prefix, args, toprint=True):
        for i in range(len(P2PTestPeers.hosts)):
            cmd = prefix + ' ' + P2PTestPeers.hosts[i] + ' ' + args
            if toprint is True:
                print('execute:{0} in host {1}'.format(cmd, P2PTestPeers.hosts[i]))
            subprocess.call(cmd, shell=True)

