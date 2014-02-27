SubD
====

A tiny subtitle downloader



Dependencies:

libcurl           
boost (libboost_filesystem, libboost_system)        
jsoncpp (included in source)      
openssl (linux)      



How to Build       
Mac OSX:        
git clone https://github.com/chainone/SubD.git    
cd SubD     
clang++ -c json-cpp/jsoncpp.cpp -Ijson-cpp    
clang++ -c main.cpp -Ijson-cpp    
clang++ json-cpp/jsoncpp.o main.o -o subd -L/usr/local/lib/ -lcurl.4 -lboost_system -lboost_filesystem    

LINUX:    
git clone https://github.com/chainone/SubD.git    
cd SubD     
g++ -c json-cpp/jsoncpp.cpp -Ijson-cpp    
g++ -c main.cpp -Ijson-cpp -std=gnu++0x    
g++ jsoncpp.o main.o -o subd -L/usr/local/lib/ -lcurl -lboost_system -lboost_filesystem    

Usage:    
subd $file1 $file2 ....    



TODO:    
1. compile windows, android and ios    
2. cmake    
3. support opensubtitles.org    
