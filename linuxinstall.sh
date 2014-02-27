#!/bin/bash

g++ -c json-cpp/jsoncpp.cpp -Ijson-cpp
g++ -c main.cpp -Ijson-cpp -std=gnu++0x
g++ jsoncpp.o main.o -o subd -L/usr/local/lib/ -lcurl -lboost_system -lboost_filesystem
