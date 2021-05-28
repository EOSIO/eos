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
$ docker-compose up -d
```

2. Check if every container is running in the local deployment 
```
docker-compose ps
```

3. If you are runing the docker-compose on the remote machine, you need ssh port forwarding *from your local machine* to access the jaeger or grafana UI
```
$ ssh -N $remote_user@$remote_ip -L 127.0.0.1:16686:127.0.0.1:16686 -L 127.0.0.1:3000:127.0.0.1:3000
```

4. Open your browser to visit http://localhost:16686 for jaeger and http://localhost:3000 for grafana

5. Bring the local deployment down
```
$ docker-compose down
```
## Run the stack in docker swam

### Set up a Docker registry on the manager node
Because a swarm consists of multiple Docker Engines, a registry is required to distribute images to all of them. You can use the Docker Hub or maintain your own. Here’s how to create a throwaway registry, which you can discard afterward.

1. Get the public IP of the manger node
```
$ MANAGER_IP=$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)
```

2. Generate your own certificate:
```
$ mkdir -p certs
$ openssl req \
  -newkey rsa:4096 -nodes -sha256 -keyout certs/ca.key \
  -subj "/CN=$MANAGER_IP/O=Block One/C=US" \
  -addext "subjectAltName = IP:${MANAGER_IP}" \
  -x509 -days 365 -out certs/ca.crt
```

3. Run and set up the registry
Run Docker Registry with created certificates. Expose it on the host port = 5443
```
$ docker run -d \
  --restart=unless-stopped \
  --name registry \
  -v $PWD/certs:/certs \
  -v /var/lib/registry:/var/lib/registry \
  -e REGISTRY_HTTP_ADDR=0.0.0.0:443 \
  -e REGISTRY_HTTP_TLS_CERTIFICATE=/certs/ca.crt \
  -e REGISTRY_HTTP_TLS_KEY=/certs/ca.key \
  -p 5443:443 \
  registry:2
```

4. Instruct every Docker daemon to trust that certificate. In order to do that copy the certificate file (ca.crt) to /etc/docker/certs.d/$MANAGER_HOSTNAME:5443/ca.crt on *every* Docker host. You do not need to restart Docker.

```
$ sudo mkdir -p /etc/docker/certs.d/$MANAGER_IP:5443
$ sudo cp certs/ca.crt /etc/docker/certs.d/$MANAGER_IP:5443/ca.crt
```

*Make sure* to copy the `ca.crt` to the `/etc/docker/certs.d/$MANAGER_IP:5443` directory in other nodes.

5. Test the registry on manager node
```
$ docker pull busybox
$ docker tag busybox $MANAGER_IP:5443/busybox
$ docker push $MANAGER_IP:5443/busybox
Using default tag: latest
The push refers to repository [ip-172-31-3-140:5443/busybox]
d0d0905d7be4: Pushed
latest: digest: sha256:f3cfc9d0dbf931d3db4685ec659b7ac68e2a578219da4aae65427886e649b06b size: 527
```

6. Test the registry on worker nodes
```
$ MANAGER_IP=34.212.138.85
$ docker pull $MANAGER_IP:5443/busybox
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
2. Run the command produced by the docker swarm init output from the Create a swarm tutorial step to create a worker node joined to the existing swarm,
  make sure to append the ` --advertise-addr $(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)` at the end of the copied command:
```
$ docker swarm join \
  --token  SWMTKN-1-49nj1cmql0jkz5s954yi3oex3nedyz0fb0xx14ie39trti4wxv-8vxv8rssmk743ojnwacrr2e7c \
  192.168.99.100:2377 --advertise-addr $(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)

This node joined a swarm as a worker.
```

### Install loki-docker-driver on all machines

```
$ docker plugin install grafana/loki-docker-driver:latest --alias loki --grant-all-permissions
```

### Label worker nodes

On the manager node
```
$ docker node ls
ID                            MANAGER_HOSTNAME          STATUS    AVAILABILITY   MANAGER STATUS   ENGINE VERSION
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
To distribute the EOS image across the swarm, it needs to be pushed to the registry you set up earlier. 

```
$ ./push_latest_image.sh
```

### Deploy the stack to the swarm

1. Create the stack with docker stack deploy:
```
$ ./swarm_deploy.sh
```

2. Check that it’s running with docker stack services rodeos-test:
```
$ docker stack ps rodeos-test 
ID             NAME                      IMAGE                                 NODE              DESIRED STATE   CURRENT STATE            ERROR     PORTS
z88zfx1hsysr   rodeos-test_bootstrap.1   34.212.138.85:5443/eos-boxed:latest   ip-172-31-3-140   Shutdown        Complete 1 second ago              
cpq5a21du4cy   rodeos-test_mysql.1       openzipkin/zipkin-mysql:latest        ip-172-31-3-140   Running         Running 26 seconds ago             
cviczxp37r5h   rodeos-test_producer.1    34.212.138.85:5443/eos-boxed:latest   ip-172-31-3-140   Running         Running 27 seconds ago             
0pbh18x8c0a6   rodeos-test_rodeos.1      34.212.138.85:5443/eos-boxed:latest   ip-172-31-12-94   Running         Running 21 seconds ago             
0p77d6smcfsb   rodeos-test_ship.1        34.212.138.85:5443/eos-boxed:latest   ip-172-31-6-31    Running         Running 8 seconds ago              
ow36qmzjrhnu   rodeos-test_zipkin.1      openzipkin/zipkin:latest              ip-172-31-3-140   Running         Running 20 seconds ago
```

3. Following the previous steps to do port forwarding and open jaeger

4. Bring the stack down with docker stack rm:
```
$ docker stack rm rodeos-test
```

The command would return immediately before everything is down. You can still check the services status with
```
$ docker stack ps rodeos-test
ID             NAME                          IMAGE                                       NODE              DESIRED STATE   CURRENT STATE           ERROR     PORTS
kck8rgws417q   6o54orzi53z3q5saz0mj36ler.1   34.212.138.85:5443/eos-boxed:d59242030c36   ip-172-31-6-31    Remove          Running 9 seconds ago             
eijcmk7a3vyp   8h1ngls93nlvd4pc11fepoafj.1   34.212.138.85:5443/eos-boxed:d59242030c36   ip-172-31-3-140   Remove          Running 9 seconds ago             
s4pqt8znken0   pnyx2z2j4ubyk2tx7j8b7esbn.1   34.212.138.85:5443/eos-boxed:d59242030c36   ip-172-31-12-94   Remove          Running 9 seconds ago             
98r1qdy09s1l   yxdku3omt08xp9i2swdtbj2h1.1   jaegertracing/jaeger-collector:latest       ip-172-31-3-140   Remove          Running 9 seconds ago     
```

Make sure every is destroyed before you deploy the next. 

5. Bring the registry down with docker service rm:
```
$ docker service rm registry
```


