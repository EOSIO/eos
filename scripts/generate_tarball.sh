#! /bin/bash

NAME=$1
EOS_PREFIX=${PREFIX}/${SUBPREFIX}
mkdir -p ${PREFIX}/bin/
#mkdir -p ${PREFIX}/lib/cmake/${PROJECT}
mkdir -p ${EOS_PREFIX}/bin 
mkdir -p ${EOS_PREFIX}/licenses/eosio
#mkdir -p ${EOS_PREFIX}/include
#mkdir -p ${EOS_PREFIX}/lib/cmake/${PROJECT}
#mkdir -p ${EOS_PREFIX}/cmake
#mkdir -p ${EOS_PREFIX}/scripts

# install binaries 
cp -R ${BUILD_DIR}/bin/* ${EOS_PREFIX}/bin 

# install licenses
cp -R ${BUILD_DIR}/licenses/eosio/* ${EOS_PREFIX}/licenses

# install libraries
#cp -R ${BUILD_DIR}/lib/* ${EOS_PREFIX}/lib

# install cmake modules
#sed "s/_PREFIX_/\/${SPREFIX}/g" ${BUILD_DIR}/modules/EosioTesterPackage.cmake &> ${EOS_PREFIX}/lib/cmake/${PROJECT}/EosioTester.cmake
#sed "s/_PREFIX_/\/${SPREFIX}\/${SSUBPREFIX}/g" ${BUILD_DIR}/modules/${PROJECT}-config.cmake.package &> ${EOS_PREFIX}/lib/cmake/${PROJECT}/${PROJECT}-config.cmake

# install includes
#cp -R ${BUILD_DIR}/include/* ${EOS_PREFIX}/include

# make symlinks
#pushd ${PREFIX}/lib/cmake/${PROJECT} &> /dev/null
#ln -sf ../../../${SUBPREFIX}/lib/cmake/${PROJECT}/${PROJECT}-config.cmake ${PROJECT}-config.cmake
#ln -sf ../../../${SUBPREFIX}/lib/cmake/${PROJECT}/EosioTester.cmake EosioTester.cmake
#popd &> /dev/null

pushd ${PREFIX}/bin &> /dev/null
for f in `ls ${BUILD_DIR}/bin/`; do
   bn=$(basename $f)
   ln -sf ../${SUBPREFIX}/bin/$bn $bn
done
popd &> /dev/null

tar -cvzf $NAME ./${PREFIX}/*
rm -r ${PREFIX}
