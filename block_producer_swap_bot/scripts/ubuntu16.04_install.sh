#!/usr/bin/env bash
sudo apt-get upgrade
apt-get update \
&& apt-get install -y software-properties-common curl \
&& add-apt-repository ppa:deadsnakes/ppa \
&& apt-get update \
&& apt-get install -y python3.6 python3.6-venv

sudo apt-get install -y build-essential python3.6-dev

sudo apt-get install -y virtualenv

export LC_ALL="en_US.UTF-8"
export LC_CTYPE="en_US.UTF-8"

virtualenv -p python3.6 swap_env
source ./swap_env/bin/activate
pip3 install -r requirements.txt
