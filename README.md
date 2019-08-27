# UOS Overview
...


## Starting up the node

Clone the code
```
git clone https://github.com/UOSnetwork/uos
```

Update the submodules
```
cd uos
git submodule update --init --recursive
```

Install the additional libs:
```
Debian/Ubuntu: apt -y install libboost-all-dev build-essential llvm-4.0-dev
CentOS7: yum -y install boost-devel
```

Compile:
```
./eosio_build.sh
```

Install:
```
./eosio_install.sh
```

Run and stop the node to generate the default config
```
nodeos
Ctrl+C
```

Add the seed nodes to the default config at ~/.local/share/eosio/nodeos/config/config.ini
```
p2p-peer-address = node-1.uos.network:9876
p2p-peer-address = node-2.uos.network:9876
p2p-peer-address = node-3.uos.network:9876
p2p-peer-address = node-4.uos.network:9876
p2p-peer-address = node-5.uos.network:9876
```

See also:

* [CONTRIBUTING](../../../uos.docs/blob/master/CONTRIBUTING.md) for detailed project information.
* [Spinning up a Node](../../../uos.docs/blob/master/uosBPubuntu.md) for a detailed node setup walkthrough on U°OS testnet.


## Contributing

[Contributing Guide](./CONTRIBUTING.md)

[Code of Conduct](./CONTRIBUTING.md#conduct)

## License

[MIT](./LICENSE)

## Important

See LICENSE for copyright and license terms.  Block.one makes its contribution on a voluntary basis as a member of the EOSIO community and is not responsible for ensuring the overall performance of the software or any related applications.  We make no representation, warranty, guarantee or undertaking in respect of the software or any related documentation, whether expressed or implied, including but not limited to the warranties or merchantability, fitness for a particular purpose and noninfringement. In no event shall we be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or documentation or the use or other dealings in the software or documentation.  Any test results or performance figures are indicative and will not reflect performance under all conditions.  Any reference to any third party or third-party product, service or other resource is not an endorsement or recommendation by Block.one.  We are not responsible, and disclaim any and all responsibility and liability, for your use of or reliance on any of these resources. Third-party resources may be updated, changed or terminated at any time, so the information here may be out of date or inaccurate.
