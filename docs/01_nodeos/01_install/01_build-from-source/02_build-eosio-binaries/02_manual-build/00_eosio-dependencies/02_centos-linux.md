## Dependencies - Manual Install - CentOS 7 and higher

Install the development toolkit:
* Installation on Centos requires installing/enabling the Centos Software Collections Repository - [Centos SCL](https://wiki.centos.org/AdditionalResources/Repositories/SCL):

```sh
$ sudo yum --enablerepo=extras install centos-release-scl
$ sudo yum update
$ sudo yum install -y devtoolset-7
$ scl enable devtoolset-7 bash
$ sudo yum install -y python33.x86_64
$ scl enable python33 bash
$ sudo yum install git autoconf automake libtool make bzip2 \
				 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 \
				 ocaml.x86_64 doxygen libicu-devel.x86_64 python-devel.x86_64 \
				 gettext-devel.x86_64

```

Install CMake 3.10.2:

```sh
$ cd ~
$ curl -L -O https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
$ tar xf cmake-3.10.2.tar.gz
$ cd cmake-3.10.2
$ ./bootstrap
$ make -j$( nproc )
$ sudo make install
```

Install Boost 1.66:

```sh
$ cd ~
$ curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
$ tar xf boost_1.66.0.tar.bz2
$ echo "export BOOST_ROOT=$HOME/boost_1_66_0" >> ~/.bash_profile
$ source ~/.bash_profile
$ cd boost_1_66_0/
$ ./bootstrap.sh "--prefix=$BOOST_ROOT"
$ ./b2 install
```

Install [MongoDB (mongodb.org)](https://www.mongodb.com):

```sh
$ mkdir ${HOME}/opt
$ cd ${HOME}/opt
$ curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-3.6.3.tgz
$ tar xf mongodb-linux-x86_64-amazon-3.6.3.tgz
$ rm -f mongodb-linux-x86_64-amazon-3.6.3.tgz
$ ln -s ${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3/ ${HOME}/opt/mongodb
$ mkdir ${HOME}/opt/mongodb/data
$ mkdir ${HOME}/opt/mongodb/log
$ touch ${HOME}/opt/mongodb/log/mongod.log

$ tee > /dev/null ${HOME}/opt/mongodb/mongod.conf <<mongodconf
systemLog:
 destination: file
 path: ${HOME}/opt/mongodb/log/mongod.log
 logAppend: true
 logRotate: reopen
net:
 bindIp: 127.0.0.1,::1
 ipv6: true
storage:
 dbPath: ${HOME}/opt/mongodb/data
mongodconf

$ export PATH=${HOME}/opt/mongodb/bin:$PATH
$ mongod -f ${HOME}/opt/mongodb/mongod.conf
```

Install [mongo-cxx-driver (release/stable)](https://github.com/mongodb/mongo-cxx-driver):

```sh
$ cd ~
$ curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
$ tar xf mongo-c-driver-1.9.3.tar.gz
$ cd mongo-c-driver-1.9.3
$ ./configure --enable-static --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
$ make -j$( nproc )
$ sudo make install
$ git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
$ cd mongo-cxx-driver/build
$ cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/$ local ..
$ sudo make -j$( nproc )
```

Install [secp256k1-zkp (Cryptonomex branch)](https://github.com/cryptonomex/secp256k1-zkp.git):

```sh
$ cd ~
$ git clone https://github.com/cryptonomex/secp256k1-zkp.git
$ cd secp256k1-zkp
$ ./autogen.sh
$ ./configure
$ make -j$( nproc )
$ sudo make install
```

By default LLVM and clang do not include the WASM build target, so you will have to build it yourself:

```sh
$ mkdir  ~/wasm-compiler
$ cd ~/wasm-compiler
$ git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
$ cd llvm/tools
$ git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
$ cd ..
$ mkdir build
$ cd build
$ cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=.. -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly 
-DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release ../
$ make -j$( nproc ) 
$ make install
```

Your environment is set up. Now you can [build EOSIO from source](../../../index.md).
