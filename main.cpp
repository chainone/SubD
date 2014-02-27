//
//  main.cpp
//  CYSubDownloader
//
//  Created by ChenYong on 2/26/14.
//  Copyright (c) 2014 ChenYong. All rights reserved.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <json/json.h>
#include <errno.h>

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define DIGEST_SAMPLE_BUF_LEN 4*1024

#define USER_AGENT "Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1667.0 Safari/537.36"
#define SHOOTER_API "https://www.shooter.cn/api/subapi.php"


std::string
compute_md5_string(const char *str, int length) {
   int n;
   MD5_CTX c;
   unsigned char digest[16];
   char out[33];
   
   MD5_Init(&c);
   MD5_Update(&c, str, length);
   MD5_Final(digest, &c);
   
   for (n = 0; n < 16; ++n) {
      snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
   }
   return out;
}


std::string
compute_shooter_file_hash(const char* file_path)
{
   assert(file_path);
   
   boost::filesystem::path path = file_path;
   int64_t file_size = boost::filesystem::file_size(path);
   
   if(file_size < 8*1024)
   {
      printf("File too small to compute the hash from file: %s\n", file_path);
      return "";
   }
   
   FILE* file_handle;
   file_handle = fopen(file_path, "rb");
   if(!file_handle)
   {
      printf("Failed to open file: %s\n", file_path);
      return "";
   }

   
   int64_t offset[4];
   offset[0] = 4*1024;
   offset[1] = file_size/3*2;
   offset[2] = file_size/3;
   offset[3] = file_size - (8*1024);
   
   char buf[1024*4];
   std::string ret="";
   
   for (int i = 0; i < 4; i++)
   {
      fseek(file_handle, offset[i], SEEK_SET);
      size_t result = fread(buf, 1, DIGEST_SAMPLE_BUF_LEN, file_handle);
      if(result!=DIGEST_SAMPLE_BUF_LEN)
      {
         printf("Reading %s error!\n", file_path);
         fclose(file_handle);
         return "";
      }
   
      ret.append(compute_md5_string(buf, DIGEST_SAMPLE_BUF_LEN));
      if(i!=3)
         ret.append(";");
   }
   
   fclose(file_handle);
   printf("Hash: %s\n", ret.c_str());
   return ret;
}


struct QueryResponse
{
   std::string response;
};

struct SubQueryLinkResult
{
   std::string ext;
   std::string link;
   void print()
   {
      printf("%s\n",link.c_str());
   }
};

struct SubQueryResult
{
   std::string des;
   int delay;
   std::vector<SubQueryLinkResult> link_results;
   void print();
};

void
SubQueryResult::print()
{
   printf("Query result:\n\tDelay:%d\n\tDescription:%s\n\tLinks:\n", delay, des.c_str());
   size_t size = link_results.size();
   for (size_t i = 0 ; i < size ; i++)
   {
      printf("\t\t");
      link_results[i].print();
   }
}

typedef std::vector<SubQueryResult> SubQueryResults;

void print(SubQueryResults& results)
{
   size_t size = results.size();
   for (size_t i = 0 ; i < size ; i++)
   {
      printf("%ld ", i);
      results[i].print();
   }
   printf("\n");
}



static size_t
receive_query_result(void *buffer, size_t size, size_t nmemb, void *userp)
{
   QueryResponse* data = static_cast<QueryResponse*>(userp);
   size_t size_in_bytes = size*nmemb;
   data->response.append((const char*)buffer, size_in_bytes);
   return size_in_bytes;
}


static std::string
compose_query_string(const char* file_path, CURL* easy_handle)
{
   std::string file_hash = compute_shooter_file_hash(file_path);
   if(!file_hash.length())
      return "";
   const char* encoded_hash = curl_easy_escape(easy_handle, file_hash.c_str(), 0);
   const char* encoded_path = curl_easy_escape(easy_handle, file_path, 0);
   char post_string[512];
   sprintf(post_string, "?filehash=%s&pathinfo=%s&format=%s&lang=%s", encoded_hash, encoded_path, "json", "Chn");
   curl_free((void*)encoded_hash);
   curl_free((void*)encoded_path);
   return post_string;
}


static void
from_json(const Json::Value& json_dict, SubQueryResult& result)
{
   assert(json_dict.isObject());
   
   result.des = json_dict["Desc"].asCString();
   result.delay = json_dict["Delay"].asInt();
   
   const Json::Value& files_array = json_dict["Files"];
   assert(files_array.isArray());
   int size = files_array.size();
   for (int i = 0; i < size; i++)
   {
      SubQueryLinkResult temp;
      const Json::Value& file_link_dict = files_array[i];
      assert(file_link_dict.isObject());
      temp.ext = file_link_dict["Ext"].asCString();
      temp.link = file_link_dict["Link"].asCString();
      result.link_results.push_back(temp);
   }
}


static bool
get_matched_sub_list(const char* file_path, SubQueryResults& results)
{
   CURL* easyhandle = curl_easy_init();
   std::string query_string = compose_query_string(file_path, easyhandle);
   if(query_string.length() == 0)
   {
      curl_easy_cleanup(easyhandle);
      return false;
   }
   
   std::string final_url = SHOOTER_API;
   final_url.append(query_string);
   
   QueryResponse query_response;

   //curl_easy_setopt(easyhandle, CURLOPT_VERBOSE,1L);
   curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 15);
   curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, USER_AGENT);
   curl_easy_setopt(easyhandle, CURLOPT_URL, final_url.c_str());
   curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, receive_query_result);
   curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &query_response);
   CURLcode success = curl_easy_perform(easyhandle);
   curl_easy_cleanup(easyhandle);
   
   if(success!=CURLE_OK)
   {
      printf("Request error, response: %s", query_response.response.c_str());
      return false;
   }
   
   //Now, parse the response
   Json::Reader reader;
   Json::Value query_result_json_array;
   if(!reader.parse(query_response.response, query_result_json_array,false))
   {
      printf("Json Parse error, response: %s", query_response.response.c_str());
      return false;
   }
   
   
   int size = query_result_json_array.size();
   assert(query_result_json_array.isArray());
   for (int i = 0 ; i < size; i++)
   {
      SubQueryResult temp;
      const Json::Value& json_dict = query_result_json_array[i];
      from_json(json_dict, temp);
      results.push_back(temp);
   }
   
   return true;
}


struct SubFileName
{
   std::string file_name;
   size_t id;
};



 static size_t
receive_header(void *ptr, size_t size, size_t nmemb, void *userdata)
{
   SubFileName& file_name_struct = *(static_cast<SubFileName*>(userdata));
   std::string& file_name = file_name_struct.file_name;
   size_t size_in_bytes = size*nmemb;
   std::string headernamevalue((const char*)ptr, size_in_bytes);
   
   if(headernamevalue.find("Content-Disposition") != std::string::npos)
   {
      size_t pos = headernamevalue.find("filename=");
      if(pos != std::string::npos)
         pos+=strlen("filename=");
      
      file_name = headernamevalue.c_str()+pos;
      boost::trim(file_name);
      pos = file_name.find_last_of('.');
      if(pos != std::string::npos)
      {
         std::string to_insert(".");
         to_insert.append(std::to_string(file_name_struct.id));
         file_name.insert(pos, to_insert);
      }
   }
   return size_in_bytes;
}

static bool
download_sub_file(const std::string& file_url, const std::string& orignal_media_file_path, const std::string& sub_ext, size_t index)
{
   boost::filesystem::path origninal_media_path(orignal_media_file_path);
   std::string  default_sub_file_name = origninal_media_path.stem().string();
   default_sub_file_name.append(".");
   default_sub_file_name.append(sub_ext);
   
   SubFileName final_file_name;
   final_file_name.id = index;
   
   std::string default_sub_file_full_path = (origninal_media_path.parent_path() /= default_sub_file_name).string();
   FILE* file = fopen(default_sub_file_full_path.c_str(), "w");
   
   if(!file)
   {
      printf("Failed to open file (%s) for writing\n", file_url.c_str());
      return false;
   }
   
   CURL* easyhandle = curl_easy_init();

   //curl_easy_setopt(easyhandle, CURLOPT_VERBOSE,1L);
   //curl_easy_setopt(easyhandle, CURLOPT_HEADER, 1L);
   curl_easy_setopt(easyhandle, CURLOPT_HEADERFUNCTION, receive_header);
   curl_easy_setopt(easyhandle, CURLOPT_HEADERDATA, &final_file_name);
   curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 15);
   curl_easy_setopt(easyhandle, CURLOPT_USERAGENT, USER_AGENT);
   curl_easy_setopt(easyhandle, CURLOPT_URL, file_url.c_str());
   curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, file);
   CURLcode success = curl_easy_perform(easyhandle);
   curl_easy_cleanup(easyhandle);
   
   fclose(file);
   
   if(success!=CURLE_OK)
   {
      printf("Download error\n");
      unlink(default_sub_file_full_path.c_str());
      return false;
   }
   
   if(final_file_name.file_name.length()>0 && final_file_name.file_name != default_sub_file_name)
   {
      //rename the file
      if(rename(default_sub_file_full_path.c_str(), (origninal_media_path.parent_path() /= final_file_name.file_name).string().c_str()))
      {
         final_file_name.file_name = default_sub_file_name;
         printf("Error happened when renaming the subtitle file\n");
      }
      
   }
   else
      final_file_name.file_name = default_sub_file_name;
   
   printf("Downloaded Sub file: %s\n", final_file_name.file_name.c_str());
   return true;
}

static void
download_sub_files(const SubQueryResult& result, const std::string& orignial_media_file_path, size_t& index)
{
   size_t size = result.link_results.size();
   if(size == 0)
      return;
   
   for (size_t i = 0; i < size; i++)
   {
      const SubQueryLinkResult& lr = result.link_results[i];
      download_sub_file(lr.link, orignial_media_file_path, lr.ext, index++);
   }
   
}

static void
download_sub_files(const SubQueryResults& results, const std::string& orignial_media_file_path, int target = -1)
{
   size_t result_size = results.size();
   if(result_size == 0)
      return;
   size_t index = 0;
   
   if(target == -1)
   {
      for (size_t i = 0; i < result_size; i++)
      {
         const SubQueryResult& r = results[i];
         download_sub_files(r, orignial_media_file_path, index);
      }
   }
   else
   {
      if(target>=result_size)
      {
         printf("The index of sub file results exceeds the size\n");
      }
      
      download_sub_files(results[target], orignial_media_file_path, index);
   }

}



static void
print_help()
{
   printf("==A tiny tool to download subtitles from shooter==\n");
   printf("usage: cysubdownloader $file1 $file2 ....\n");
}

int
main(int argc, const char * argv[])
{
   bool help = false;
   if(argc == 1)
      help = true;
   else if(argc == 2)
   {
      if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0)
         help = true;
   }
   
   if(help)
      print_help();
   
   SubQueryResults results;
   boost::filesystem::path prefered_path;
   std::string prefered_path_string;
   
   curl_global_init(0L);
   
   for (int i = 1; i < argc; i++)
   {
      prefered_path = argv[i];
      
      if ( !boost::filesystem::exists( prefered_path ) )
      {
         printf("not found: %s\n", prefered_path.c_str());
         return 1;
      }
      
      if ( !boost::filesystem::is_regular( prefered_path ) )
      {
         printf("not a regular file: %s\n", prefered_path.c_str());
         return 1;
      }
      
      prefered_path_string = prefered_path.make_preferred().string();
      
      printf("Start query subtitles for %s\n", prefered_path_string.c_str());
      get_matched_sub_list(prefered_path_string.c_str(), results);
      print(results);
      printf("\nStart to download subtitles....\n");
      
      download_sub_files(results, prefered_path_string);
      
      printf("Done.\n");
   }
   
   curl_global_cleanup();
   

    return 0;
}

