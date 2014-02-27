#!/bin/bash

g++ -c json-cpp/jsoncpp.cpp -Ijson-cpp -std=gnu++0x -D_FILE_OFFSET_BITS=64
g++ -c main.cpp -Ijson-cpp -std=gnu++0x -D_FILE_OFFSET_BITS=64
g++ jsoncpp.o main.o -o subd -L/usr/local/lib/ -lcurl -lboost_system -lboost_filesystem 
