#!/usr/bin/env bash
sudo apt upgrade
sudo apt update
sudo apt install build-essential
sudo apt install python3-dev
sudo apt install virtualenv

virtualenv -p python3.6 swap_env
source ./swap_env/bin/activate
pip3 install -r requirements.txt
