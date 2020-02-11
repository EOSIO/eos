import subprocess

class P2PTestPeers:

    #for testing with localhost
    sshname = "testnet" # ssh name for executing remote commands
    hosts = ["localhost"] # host list
    ports = [8888] # eosiod listening port of each host
    devs = ["lo0"] # network device of each host

    #for testing with testnet2
    #sshname = "testnet" # ssh name for executing remote commands
    #hosts = ["10.160.11.101", "10.160.12.102", "10.160.13.103", "10.160.11.104", "10.160.12.105", "10.160.13.106", "10.160.11.107", "10.160.12.108", "10.160.13.109", "10.160.11.110", "10.160.12.111", "10.160.13.112", "10.160.11.113", "10.160.12.114", "10.160.13.115", "10.160.11.116", "10.160.12.117", "10.160.13.118", "10.160.11.119", "10.160.12.120", "10.160.13.121", "10.160.11.122"] # host list
    #ports = [8888, 8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888,8888, 8888] # eosiod listening port of each host
    #devs = ["ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3", "ens3"] # network device of each host

    @staticmethod
    def exec(remoteCmd, toprint=True):
        for i in range(len(P2PTestPeers.hosts)):
            remoteCmd2 = remoteCmd.replace("{dev}", P2PTestPeers.devs[i])
            cmd = "ssh " + P2PTestPeers.sshname + "@" + P2PTestPeers.hosts[i] + ' ' + remoteCmd2
            if toprint is True:
                print("execute:" + cmd)
            subprocess.call(cmd, shell=True)