#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <regex>
#include <condition_variable>

#include "connect_mgr.hpp"

using namespace std;

namespace eosio {
namespace blockvault_client{

   sptr<bv_client_connection_base> bv_client_connection_manager::connect_mgr_instance = nullptr;

   static  sptr<bv_client_connection_base> bv_client_connection_manager::get_bvcc_manager_instance(){
      if(connect_mgr_instance == nullptr){
         connect_mgr_instance = std::make_shared<bv_client_connection_manager>();
      } 
      return connect_mgr_instance;
   }
   
   static  sptr<bv_client_connection_base> bv_client_connection_manager::get_bvcc_manager_instance(const string & config_file_path){
      if(connect_mgr_instance == nullptr){
         connect_mgr_instance = std::make_shared<bv_client_connection_manager>(config_file_path);
      } 
      return connect_mgr_instance;
   }
   
   bv_client_connection_manager::bv_client_connection_manager(){
      m_config_file_path = std::string(CLUSTER_INIT_FILE_PATH);
      m_cluster_config = get_init_config(m_config_file_path);
      // for debug only
      for(const auto & p : m_cluster_config){
         cout << p.first << ":" << p.second << endl;
      }
   }
   
   bv_client_connection_manager::bv_client_connection_manager(const string & config_file_path){
      m_config_file_path = config_file_path;
      m_cluster_config = get_init_config(m_config_file_path);
   }
   
   virtual ~bv_client_connection_manager::bv_client_connection_manager(){
   }
   
   vector<pair<string,string>> bv_client_connection_manager::get_init_config(const string& config_file_path){  // fetch init cluster from file      
      vector<pair<string,string>> cluster_config;
      std::ifstream infile(config_file_path);
      std::string line;
      while (std::getline(infile, line)){
         regex re_ip_port("^(\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}):(\\d{1,5})$");
         smatch m;
         if(regex_match(line, m, re_ip_port)){
            cluster_config.push_back({m[1],m[2]});	  
         }
      }
      return cluster_config;
   }
   
   vector<pair<string, string>> bv_client_connection_manager::get_config_from_cluster(){ //  pull updated endpoints from cluster, by http request
     vector<pair<string,string>> cluster_config;
     return cluster_config;
   }
   
   bool bv_client_connection_manager::set_config(const vector<pair<string, string>> &  config){
      #ifdef CONNECT_MGR_THREAD_SAFE
      m_config_mutex.lock();
      #endif
      m_cluster_config = config;
      #ifdef CONNECT_MGR_THREAD_SAFE
      m_config_mutex.unlock();
      #endif
      return true;
   }
   
   sptr<sync_response> bv_client_connection_manager::sync_send(bv_client_connnection, sptr<sync_response> pframe, int frame_len){ // sync call wait until server response
   }
   
   bool bv_client_connection_manager::async_send(bv_client_connnection, sptr<sync_response> pframe, int frame_len){ // return after push frame to queue, no wait, will fail if queue full
   }
   
   sptr<sync_response> bv_client_connection_manager::async_get_response(bv_client_connnection bvc_id){  // called by client's callback function
   }
   
   virtual bv_client_connnection & bv_client_connection_manager::create_connection(){
   }
   virtual bv_client_connnection & create_connection(string ip, string port){
   }
   virtual bv_client_connnection & create_leader_connection(){
   }


}
}
