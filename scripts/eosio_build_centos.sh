echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

( [[ $NAME == "CentOS Linux" ]] && [[ "$(echo ${VERSION} | sed 's/ .*//g')" < 7 ]] ) && echo " - You must be running Centos 7 or higher to install EOSIO." && exit 1

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

echo ""

if [[ "$(echo ${VERSION} | sed 's/ .*//g')" == 7 ]]; then
# Repo necessary for rh-python3, devtoolset-8 and llvm-toolset-7.0
ensure-scl
# GCC8 for Centos / Needed for CMAKE install even if we're pinning
ensure-devtoolset
if [[ -d /opt/rh/devtoolset-8 ]]; then
	echo "${COLOR_CYAN}[Enabling Centos devtoolset-8 (so we can use GCC 8)]${COLOR_NC}"
	execute-always source /opt/rh/devtoolset-8/enable
	echo " - ${COLOR_GREEN}Centos devtoolset-8 successfully enabled!${COLOR_NC}"
fi

# Ensure packages exist
ensure-yum-packages "${REPO_ROOT}/scripts/eosio_build_centos7_deps"
export PYTHON3PATH="/opt/rh/rh-python36"
if $DRYRUN || [ -d $PYTHON3PATH ]; then
	echo "${COLOR_CYAN}[Enabling python36]${COLOR_NC}"
	execute source $PYTHON3PATH/enable
	echo " ${COLOR_GREEN}- Python36 successfully enabled!${COLOR_NC}"
	echo ""
fi
fi

if [[ "$(echo ${VERSION} | sed 's/ .*//g')" == 8 ]]; then
        echo "Install Development Tools ..."
        install-package https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
        group-install-package 'Development Tools'
        install-package openssl-devel
        install-package which
	install-package git 
	install-package autoconf
	install-package automake
	install-package libtool
	install-package make
	install-package bzip2
    	install-package graphviz
	install-package bzip2-devel
	install-package openssl-devel
	install-package gmp-devel
    	install-package file
	install-package libusbx-devel
    	install-package libcurl-devel
	install-package patch
        install-package vim-common
        install-package jq
    	install-package python3
	install-package python3-devel
	install-package clang
	install-package llvm-devel
	install-package llvm-static
	install-package procps-ng
	install-package util-linux
        install-package sudo
	install-package libstdc++
        install-package dnf-plugins-core
        sudo dnf config-manager --set-enabled PowerTools || sudo dnf config-manager --set-enabled powertools
        install-package doxygen
        install-package ocaml
	install-package ncurses-compat-libs
        install-package nodejs
        install-package epel-release

        curl --silent --location https://dl.yarnpkg.com/rpm/yarn.repo | sudo tee /etc/yum.repos.d/yarn.repo
        sudo rpm --import https://dl.yarnpkg.com/rpm/pubkey.gpg
        install-package yarn

        pushd ${REPO_ROOT}
        yarn install
        popd

        if [ ! -L "/usr/local/lib/libtinfo.so" ] ; then
	    ln -s /usr/lib64/libtinfo.so.6 /usr/local/lib/libtinfo.so
        fi
fi

# Handle clang/compiler
ensure-compiler
# CMAKE Installation
ensure-cmake
# CLANG Installation
build-clang
# LLVM Installation
ensure-llvm
# BOOST Installation
ensure-boost
# `libpq` and `libpqxx` Installation
ensure-libpq-and-libpqxx
