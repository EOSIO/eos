# MacOS 10.14 (Native Compiler)
The following commands will install all of the necessary dependencies for source installing EOSIO.
<!-- The code within the following block is used in our CI/CD. It will be converted line by line into RUN statements inside of a temporary Dockerfile and used to build our docker tag for this OS. 
Therefore, COPY and other Dockerfile-isms are not permitted. -->
## Clone EOSIO Repository
<!-- CLONE -->
```
brew update && brew install git
export EOSIO_LOCATION=$HOME/eosio
git clone https://github.com/EOSIO/eos.git $EOSIO_LOCATION
cd $EOSIO_LOCATION && git submodule update --init --recursive
export EOSIO_INSTALL_LOCATION=$EOSIO_LOCATION/install
mkdir -p $EOSIO_INSTALL_LOCATION
```
<!-- CLONE END -->
## Install Dependencies
<!-- DEPS -->
```
brew install cmake python@2 python libtool libusb graphviz automake wget gmp pkgconfig doxygen openssl@1.1 jq boost || :
PATH=$EOSIO_INSTALL_LOCATION/bin:$PATH
```
<!-- DEPS END -->
## Build EOSIO
<!-- BUILD -->
```
mkdir -p $EOSIO_LOCATION/build
cd $EOSIO_LOCATION/build
cmake -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_LOCATION ..
make -j$(getconf _NPROCESSORS_ONLN)
```
<!-- BUILD END -->
## Install EOSIO
<!-- INSTALL -->
```
make install
```
<!-- INSTALL END -->
## Test EOSIO
<!-- TEST -->
```
make test
```
<!-- TEST END -->
## Uninstall EOSIO
<!-- UNINSTALL -->
```
xargs rm < $EOSIO_LOCATION/build/install_manifest.txt
rm -rf $EOSIO_LOCATION/build
```
<!-- UNINSTALL END -->