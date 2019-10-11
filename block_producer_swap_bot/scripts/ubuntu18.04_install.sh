#!/usr/bin/env bash
sudo apt upgrade
sudo apt update
sudo apt install -y build-essential
sudo apt install -y python3-dev
sudo apt install -y virtualenv

virtualenv -p python3.6 swap_env
source ./swap_env/bin/activate
pip3 install -r requirements.txt
