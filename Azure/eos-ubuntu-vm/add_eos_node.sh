#!/bin/bash

set -e 

date
ps axjf

USER_NAME=$1
FQDN=$2
PRODUCER_NAME=$3
NPROC=$(nproc)
LOCAL_IP=`ifconfig|xargs|awk '{print $7}'|sed -e 's/[a-z]*:/''/'`
RPC_PORT=8888
P2P_PORT=9876
GITHUB_REPOSITORY=https://github.com/ryanrfox/eos.git
PROJECT=eos
BRANCH=$4
VALIDATOR_NODE=eosd
CLI=eosc
WALLET=eos-walletd

echo "USER_NAME: $USER_NAME"
echo "PRODUCER_NAME : $PRODUCER_NAME"
echo "FQDN: $FQDN"
echo "nproc: $NPROC"
echo "eth0: $LOCAL_IP"
echo "P2P_PORT: $P2P_PORT"
echo "RPC_PORT: $RPC_PORT"
echo "GITHUB_REPOSITORY: $GITHUB_REPOSITORY"
echo "PROJECT: $PROJECT"
echo "BRANCH: $BRANCH"
echo "VALIDATOR_NODE: $VALIDATOR_NODE"
echo "CLI: $CLI"
echo "WALLET: $WALLET"

##################################################################################################
# Update Ubuntu, configure a 2GiB swap file                                                      #
##################################################################################################
apt-get -y update || exit 1;
sleep 5;
fallocate -l 2g /mnt/2GiB.swap
chmod 600 /mnt/2GiB.swap
mkswap /mnt/2GiB.swap
swapon /mnt/2GiB.swap
echo '/mnt/2GiB.swap swap swap defaults 0 0' | tee -a /etc/fstab

##################################################################################################
# Build the EOS.IO project using the provided build script in the source repository.             #
##################################################################################################
cd /usr/local/src
time git clone $GITHUB_REPOSITORY --recursive
cd $PROJECT
time git checkout $BRANCH

echo "\n>> Begin EOS.IO installation"
bash build.sh ubuntu full

echo "\n>> Copy the EOS.IO applications to the /usr/bin folder"
cp /usr/local/src/$PROJECT/build/programs/$VALIDATOR_NODE/$VALIDATOR_NODE /usr/bin/$VALIDATOR_NODE
cp /usr/local/src/$PROJECT/build/programs/$CLI/$CLI /usr/bin/$CLI

##################################################################################################
# Configure eosd service. Enable it to start on boot.                                            #
##################################################################################################
cat >/lib/systemd/system/$VALIDATOR_NODE.service <<EOL
[Unit]
Description=Job that runs $VALIDATOR_NODE daemon
[Service]
Type=simple
Environment=statedir=/home/$USER_NAME/$PROJECT/$VALIDATOR_NODE
ExecStartPre=/bin/mkdir -p /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE
ExecStart=/usr/bin/$VALIDATOR_NODE --data-dir /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE

TimeoutSec=300
[Install]
WantedBy=multi-user.target
EOL

##################################################################################################
# Start the service, allowing creation of the default application configuration file. Stop the   #
# service to allow modification of the config.ini file.                                          #
##################################################################################################
systemctl daemon-reload
systemctl enable $VALIDATOR_NODE
service $VALIDATOR_NODE start
sleep 5; # allow time to initializize application data.
service $VALIDATOR_NODE stop

# Load the testnet genesis state, which creates some initial block producers with the default key
cp /usr/local/src/$PROJECT/genesis.json /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE
sed -i 's%# genesis-json =%genesis-json = '/home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/genesis.json'%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

 # Enable production on a stale chain, since a single-node test chain is pretty much always stale
sed -i 's%enable-stale-production = false%enable-stale-production = true%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

# Enable all 21 testnet block producers on this node
sed -i 's%# producer-name =%producer-name = inita\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initb\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initc\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initd\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = inite\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initf\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initg\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = inith\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initi\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initj\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initk\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initl\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initm\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initn\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initp\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initq\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initr\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = inits\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initt\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# producer-name =%producer-name = initu\n# producer-name =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

# Load the block producer plugin, so you can produce blocks
sed -i 's%# plugin =%plugin = eos::producer_plugin\n# plugin =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

# Wallet plugin
sed -i 's%# plugin =%plugin = eos::wallet_api_plugin\n# plugin =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

# As well as API and HTTP plugins
sed -i 's%# plugin =%plugin = eos::chain_api_plugin\n# plugin =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini
sed -i 's%# plugin =%plugin = eos::http_plugin\n# plugin =%g' /home/$USER_NAME/$PROJECT/$VALIDATOR_NODE/config.ini

##################################################################################################
# Start the service again, now using the updated config.ini settings. The 21 block producers     #
# should begin advancing the blockchain. Use the following command to view the latest stats:     #
#   /usr/bin/eosc -p 8888 get info                                                               #
##################################################################################################
service $VALIDATOR_NODE start
