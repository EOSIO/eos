#!/bin/bash

rm -Rf ./datas/bios/nodeos/*
rm -Rf ./datas/bios/keosd/*

rm -Rf ./datas/bp1/nodeos/*
rm -Rf ./datas/bp1/keosd/*
rm -Rf ./datas/bp1/blocks/*

rm -Rf ./datas/bp2/nodeos/*
rm -Rf ./datas/bp2/keosd/*
rm -Rf ./datas/bp2/blocks/*

rm -Rf ./datas/bp3/nodeos/*
rm -Rf ./datas/bp3/keosd/*
rm -Rf ./datas/bp3/blocks/*

rm -Rf ./datas/voter/nodeos/*
rm -Rf ./datas/voter/keosd/*
rm -Rf ./datas/voter/blocks/*

docker rm -f $(docker ps -aq) 
