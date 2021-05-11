## Prerequsite

- Ubuntu 20.04 machines
- Install docker 
```
$ curl -fsSL https://get.docker.com -o get-docker.sh
$ sudo sh get-docker.sh
$ sudo usermod -aG docker $USER
```
- Install docker-compose
```
$ sudo curl -L "https://github.com/docker/compose/releases/download/1.29.1/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
$ sudo chmod +x /usr/local/bin/docker-compose
```

### Test with Compose

1. Start the local deployment with docker-compose
```
$ export DOCKER_REPO=$(curl http://169.254.169.254/latest/meta-data/public-ipv4)
$ docker-compose up -d
```

2. Check if every container is running in the local deployment 
```
docker-compose ps
```
3. To access the Zipkin UI from your local machine, you need ssh port forwarding *from your local machine*
```
$ ssh -L 127.0.0.1:9411:127.0.0.1:9411 root@${MANAGER_IP}
```
4. Open your browser to visit http://127.0.0.1:9411

5. Bring the local deployment down
```
$ docker-compose down
```
## Run the stack in docker swam

### Set up a Docker registry
Because a swarm consists of multiple Docker Engines, a registry is required to distribute images to all of them. You can use the Docker Hub or maintain your own. Here’s how to create a throwaway registry, which you can discard afterward.

1. Start the registry as a service on your manger node:
```
docker service create --name registry --publish published=5000,target=5000 registry:2
```

2. Check its status with docker service ls:
```
$ docker service ls

ID            NAME      REPLICAS  IMAGE                                                                               COMMAND
l7791tpuwkco  registry  1/1       registry:2@sha256:1152291c7f93a4ea2ddc95e46d142c31e743b6dd70e194af9e6ebe530f782c17
```

3. Check that it’s working with curl:
```
$ curl http://localhost:5000/v2/

{}
```

### Create the swam

Run the following command on the machine which is designated as the manager node
```
$ MANAGER_IP=$(curl http://169.254.169.254/latest/meta-data/public-ipv4)
$ docker swarm init --advertise-addr $MANAGER_IP
Swarm initialized: current node (dxn1zf6l61qsb1josjja83ngz) is now a manager.

To add a worker to this swarm, run the following command:

    docker swarm join \
    --token SWMTKN-1-49nj1cmql0jkz5s954yi3oex3nedyz0fb0xx14ie39trti4wxv-8vxv8rssmk743ojnwacrr2e7c \
    192.168.99.100:2377

To add a manager to this swarm, run 'docker swarm join-token manager' and follow the instructions.
```

### Add nodes to the swarm
Once you’ve created a swarm with a manager node, you’re ready to add worker nodes.
1. Open a terminal and ssh into the machine where you want to run a worker node. This tutorial uses the name worker1.
2. Run the command produced by the docker swarm init output from the Create a swarm tutorial step to create a worker node joined to the existing swarm:
```
$ docker swarm join \
  --token  SWMTKN-1-49nj1cmql0jkz5s954yi3oex3nedyz0fb0xx14ie39trti4wxv-8vxv8rssmk743ojnwacrr2e7c \
  192.168.99.100:2377

This node joined a swarm as a worker.
```

### Label worker nodes

On the manager node
```
$ docker node ls
ID                            HOSTNAME          STATUS    AVAILABILITY   MANAGER STATUS   ENGINE VERSION
sklu2flyoocofz87a6bcwask3 *   ip-172-31-3-140   Ready     Active         Leader           20.10.6
yb1j805cjyv5yurhere8lndyd     ip-172-31-6-31    Ready     Active                          20.10.6
csuy1nxb0es6zbo3t6j8osu9z     ip-172-31-12-94   Ready     Active                          20.10.6
```

Now creat labels for the non-leader nodes

```
$ docker node update --label-add name=node-1 yb1j805cjyv5yurhere8lndyd
$ docker node update --label-add name=node-2 csuy1nxb0es6zbo3t6j8osu9z
```

### Push the generated image to the registry
To distribute the web app’s image across the swarm, it needs to be pushed to the registry you set up earlier. With Compose, this is very simple:

```
$ docker-compose push
```

### Deploy the stack to the swarm

1. Create the stack with docker stack deploy:
```
$ export DOCKER_REPO=$(curl http://169.254.169.254/latest/meta-data/public-ipv4)
$ docker stack apply -c docker-compose.yaml -c cluster.yaml rodeos-test
```

2. Check that it’s running with docker stack services stackdemo:
```
$ docker stack services rodeos-test
```

3. Following the previous steps to do port forwarding and open zipkin

4. Bring the stack down with docker stack rm:
```
$ docker stack rm rodeos-test
```

5. Bring the registry down with docker service rm:
```
$ docker service rm registry
```


