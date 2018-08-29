#!/bin/bash

odb --std c++11 -d mysql -I/root/opt/boost/include/ -p boost/date-time --generate-query --generate-prepared --generate-schema --schema-format embedded eos.hpp
