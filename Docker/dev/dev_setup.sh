#!/bin/bash
# Remove comments
env=$(sed 's/#.*//g' ./.env)
# Setup Args
build_args=()
for line in $env; do
	build_args+=" --build-arg $line"
done
#!/bin/bash
docker volume create --name=nodeos-dev-data-volume
docker volume create --name=keosd-dev-data-volume
if [ ! -h ~/keosd-dev-data-volume ]; then ln -s ~/nodeos-dev-data-volume ~/keosd-dev-data-volume; fi
docker-compose -f docker-compose-dev.yml up -d
printf "\nPlease ensure that you place your genesis.json file, if necessary, into ~/nodeos-dev-data-volume, then restart the container\n"
# Ensure nodeos is started
docker start nodeos_dev
