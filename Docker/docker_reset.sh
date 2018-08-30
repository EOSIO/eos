#!/bin/bash
printf "This will delete EVERYTHING (containers, images, and volumes). However, it will not delete your data volumes! Are you sure you want to proceed?\n(Enter an integer)\n"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) docker stop $(docker ps -a -q); docker rm $(docker ps -a -q); docker images | grep  -v "eosio/builder" | grep -v IMAGE | awk '{print $3}' | xargs docker rmi; docker volume ls | awk '{print $2}' | xargs docker volume rm; exit;;
        No ) exit;;
    esac
done
