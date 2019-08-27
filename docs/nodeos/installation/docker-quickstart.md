# Docker Quickstart

[[warning | The Docker image is no longer maintained and has been deprecated. The eosio/eos image from Dockerhub will be available until its sunset on June 29 2018]]

[[warning | This guide is intended for development environments, if you intend to run EOSIO in a production environment, consider [building EOSIO](doc:autobuild-script) or using the provided [binaries](https://developers.eos.io/eosio-home/docs/setting-up-your-environment).]]

If you don't already have docker installed, you can download it here: https://www.docker.com/community-edition

# Step 1. Pull the docker image

The below statement will download an Ubuntu image which contains the compiled software. 

```
docker pull eosio/eos-dev:v1.5.2
```

# Step 2. Create Network

Here we'll use docker's `network` command to create a network for nodeos and keosd to share. 

```
docker network create eosdev
```

# Step 3. Boot Containers

## Nodeos (Core Daemon) 

```
$ docker run \
  --name nodeos -d -p 8888:8888 \
  --network eosdev \
  -v /tmp/eosio/work:/work \
  -v /tmp/eosio/data:/mnt/dev/data \
  -v /tmp/eosio/config:/mnt/dev/config \
  eosio/eos-dev \
/bin/bash -c \
  "nodeos -e -p eosio \
    --plugin eosio::producer_plugin \
    --plugin eosio::history_plugin \
    --plugin eosio::chain_api_plugin \
    --plugin eosio::history_api_plugin \
    --plugin eosio::http_plugin \
    -d /mnt/dev/data \
    --config-dir /mnt/dev/config \
    --http-server-address=0.0.0.0:8888 \
    --access-control-allow-origin=* \
    --contracts-console \
    --http-validate-host=false"
```

These settings accomplish the following:
1. Forward port 8888
2. Connect to `eosdev` local network you created in previous step
3. Alias three volumes on your local drive to the docker container. 
4. Run the Nodeos startup in bash. This command loads all the basic plugins, set the server address, enable CORS and add some contract debugging. 
5. Mount some directories in `/tmp` on local machine, changes to these folders will persist in docker container. (for example, you'll want to place contracts in `/tmp/eosio/work` so you can upload contracts to your node. 

## Run Keosd (Wallet and Keystore) 

```
docker run -d --name keosd --network=eosdev \
-i eosio/eos-dev /bin/bash -c "keosd --http-server-address=0.0.0.0:9876"
```

# Step 4. Check your installation

## Check that Nodeos is Producing Blocks

Run the following command

```
docker logs --tail 10 nodeos
```

You should see some output in the console that looks like this:

```
1929001ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366974ce4e2a... #13929 @ 2018-05-23T16:32:09.000 signed by eosio [trxs: 0, lib: 13928, confirmed: 0]
1929502ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366aea085023... #13930 @ 2018-05-23T16:32:09.500 signed by eosio [trxs: 0, lib: 13929, confirmed: 0]
1930002ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366b7f074fdd... #13931 @ 2018-05-23T16:32:10.000 signed by eosio [trxs: 0, lib: 13930, confirmed: 0]
1930501ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366cd8222adb... #13932 @ 2018-05-23T16:32:10.500 signed by eosio [trxs: 0, lib: 13931, confirmed: 0]
1931002ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366d5c1ec38d... #13933 @ 2018-05-23T16:32:11.000 signed by eosio [trxs: 0, lib: 13932, confirmed: 0]
1931501ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366e45c1f235... #13934 @ 2018-05-23T16:32:11.500 signed by eosio [trxs: 0, lib: 13933, confirmed: 0]
1932001ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000366f98adb324... #13935 @ 2018-05-23T16:32:12.000 signed by eosio [trxs: 0, lib: 13934, confirmed: 0]
1932501ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 00003670a0f01daa... #13936 @ 2018-05-23T16:32:12.500 signed by eosio [trxs: 0, lib: 13935, confirmed: 0]
1933001ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 00003671e8b36e1e... #13937 @ 2018-05-23T16:32:13.000 signed by eosio [trxs: 0, lib: 13936, confirmed: 0]
1933501ms thread-0   producer_plugin.cpp:585       block_production_loo ] Produced block 0000367257fe1623... #13938 @ 2018-05-23T16:32:13.500 signed by eosio [trxs: 0, lib: 13937, confirmed: 0]
```

Go ahead and exit that bash with `ctrl-c` or type `exit` and press enter.


## Check the Wallet

Open nodeos shell

```
docker exec -it keosd bash
```

Run the following command 

```
cleos --wallet-url http://127.0.0.1:9876 wallet list keys
```

If you see the message 

```
Wallets:
[]
```

Now that we are certain `keosd` is running correctly, type `exit` and then press enter to leave the `keosd` shell. From this point forward, we won't need to enter the containers with bash, and you'll be executing commands from your local system (Linux or Mac) 

## Check Nodeos endpoints

This will check that the RPC API is working correctly, pick one. 

1. Check the `get_info` endpoint provided by the `chain_api_plugin` in your browser: [http://localhost:8888/v1/chain/get_info](http://localhost:8888/v1/chain/get_info)
2. Check the same thing, but in your console 

```
curl http://localhost:8888/v1/chain/get_info
```

# Step 5. Alias Cleos

We won't want to enter into the Docker container's bash every time we want to interact with Nodeos or Keosd.  So let's go ahead and make cleos a bit easier to use. 

Firstly, we need to find the IP address of keosd, by the following:

```
docker network inspect eosdev
```

You should see something like this below and be able to find the IP address of keosd:

```
[
    {
        "Name": "eosdev",
        "Id": "b24a6ae0b8a559212a1bf94cac77a17748cdb98970f4ff909d6084ef966ebf23",
        "Created": "2018-09-05T01:20:26.4181748Z",
        "Scope": "local",
        "Driver": "bridge",
        "EnableIPv6": false,
        "IPAM": {
            "Driver": "default",
            "Options": {},
            "Config": [
                {
                    "Subnet": "172.18.0.0/16",
                    "Gateway": "172.18.0.1"
                }
            ]
        },
        "Internal": false,
        "Attachable": false,
        "Ingress": false,
        "ConfigFrom": {
            "Network": ""
        },
        "ConfigOnly": false,
        "Containers": {
            "2b6d8421243f8b02d19caf84735f05118f90e35f35b0ba0a035bbb045c64d707": {
                "Name": "nodeos",
                "EndpointID": "164d47d6f79b4b7348154485a6c79d9762e66d5c70cfeb39af7c7e55e7d7369f",
                "MacAddress": "02:42:ac:12:00:02",
                "IPv4Address": "172.18.0.2/16",
                "IPv6Address": ""
            },
            "ffe5f373dffdf9b31b69394416cadb7e292aa8030c6b66bd03ec90d316387dad": {
                "Name": "keosd",
                "EndpointID": "ac8c6bf49ba698c226631b41d98cb49f7289ed37f369da74ab8dd62d3dd5fa76",
                "MacAddress": "02:42:ac:12:00:03",
                "IPv4Address": "172.18.0.3/16",
                "IPv6Address": ""
            }
        },
        "Options": {},
        "Labels": {}
    }
]
```

```
alias cleos='docker exec -it nodeos /opt/eosio/bin/cleos --url http://127.0.0.1:8888 --wallet-url http://[keosd_ip]:9876'
```

What this does is create an alias to `cleos` on your local system, and links that alias to a command that is executed inside the `nodeos` container. This is why the `--url` parameter is referencing localhost on port 8888, while the `--wallet-url` references `keosd`
