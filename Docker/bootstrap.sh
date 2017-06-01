mkdir -m 777 secp
mv PKGBUILD secp
cd secp
su -s /bin/bash -c makepkg nobody
pacman --noconfirm -U libsecp256k1-zkp-cnx-20150529-1-x86_64.pkg.tar.xz
cd ..
rm -rf secp Dockerfile README.md bootstrap.sh
git clone https://github.com/eosio/eos --recursive
cd eos
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang .
ninja
