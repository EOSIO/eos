!/bin/bash
set -e
MANAGER_IP=$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)
IMAGE_ID=$(docker images eos-boxed:latest -q)
export EOS_IMAGE=$MANAGER_IP:5443/eos-boxed:${IMAGE_ID}
docker stack deploy -c docker-compose.yaml -c cluster.yaml rodeos-test