SubD
====

A tiny subtitle downloader    
    
一个从射手网下载字幕的命令行小工具    
已经跟iambus的迅雷离线脚本集成: https://github.com/chainone/xunlei-lixian    
在OSX上以及我的NAS上(ESXI + Ubuntu)常年服役    

Dependencies:

libcurl           
boost (libboost_filesystem, libboost_system)        
jsoncpp (included in source)      
openssl (linux)      

How to Build       
Mac OSX (10.9.1):        
./osxinstall.sh    

LINUX (12.04 i386 & 13.10 i386):    
./linuxinstall.sh     

Usage:    
subd $file1 $file2 ....    


TODO:    
1. compile windows, android and ios    
2. cmake    
3. support opensubtitles.org    
