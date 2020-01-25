#!/bin/bash

if [ $# -ne 1 ]; then
    echo Usage: $0 my_new_plugin
    echo ... where my_new_plugin is the name of the plugin you want to create
    exit 1
fi

pluginName=$1

echo Copying template...
cp -R template_plugin $pluginName

echo Renaming files/directories...
mv $pluginName/include/eosio/template_plugin $pluginName/include/eosio/$pluginName
for file in `find $pluginName -type f -name '*template_plugin*'`; do mv $file `sed s/template_plugin/$pluginName/g <<< $file`; done;

echo Renaming in files...
for file in `find $pluginName -type f`; do sed -i.bak -e "s/template_plugin/$pluginName/g" "$file" && rm "$file.bak"; done

echo "Done! $pluginName is ready. Don't forget to add it to CMakeLists.txt!"
