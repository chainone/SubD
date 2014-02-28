#!/bin/bash

clang++ -c json-cpp/jsoncpp.cpp -Ijson-cpp
clang++ -c main.cpp -Ijson-cpp -DENABLE_SSL
clang++ jsoncpp.o main.o -o subd -L/usr/local/lib/ -lcurl.4 -lboost_system -lboost_filesystem
