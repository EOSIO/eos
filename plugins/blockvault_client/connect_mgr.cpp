#include <cstdlib>
#include <iostream>
#include <fstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <mutex>
#include <condition_variable>
#include <string>
#include <regex>
#include <eosio/blockvault_client/connect_mgr.hpp>


namespace eosio {
namespace blockvault_client{

   sptr<bv_client_connection_manager> bv_client_connection_manager::connect_mgr_instance = nullptr;

   sptr<bv_client_connection_manager> bv_client_connection_manager::get_bvcc_manager_instance(){
      if(connect_mgr_instance == nullptr){
         connect_mgr_instance = sptr<bv_client_connection_manager>(new bv_client_connection_manager());
      } 
      return connect_mgr_instance;
   }
   
   sptr<bv_client_connection_manager> bv_client_connection_manager::get_bvcc_manager_instance(const std::string & config_file_path){
      if(connect_mgr_instance == nullptr){
         connect_mgr_instance = sptr<bv_client_connection_manager>(new bv_client_connection_manager(config_file_path));
      } 
      return connect_mgr_instance;
   }
   
   bv_client_connection_manager::bv_client_connection_manager(){
      m_config_file_path = std::string(CLUSTER_INIT_FILE_PATH);
      m_cluster_config = get_init_config(m_config_file_path);
      // for debug only
      for(const auto & p : m_cluster_config){
         std::cout << p.first << ":" << p.second << std::endl;
      }
   }
   
   bv_client_connection_manager::bv_client_connection_manager(const std::string & config_file_path){
      m_config_file_path = config_file_path;
      m_cluster_config = get_init_config(m_config_file_path);
   }
   
   bv_client_connection_manager::~bv_client_connection_manager(){
   }
   
   std::vector<std::pair<std::string, std::string>> bv_client_connection_manager::get_init_config(const std::string& config_file_path){  // fetch init cluster from file      
      std::vector<std::pair<std::string, std::string>> cluster_config;
      std::ifstream infile(config_file_path);
      std::string line;
      int line_num = 0;
      while (std::getline(infile, line)){
         std::regex re_ip_port("^(\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}):(\\d{1,5})$");
         std::smatch m;
         ++line_num;
         if(std::regex_match(line, m, re_ip_port)){
            cluster_config.push_back({m[1],m[2]});	  
         } else {
            std::cout << "Parse " << config_file_path <<  " error with line " << line_num << " : " << line << std::endl;
            std::cout << "Try remove unexpected character as blank, tab etc, only keep digit, dot, colon" << std::endl;
         }
      }
      return cluster_config;
   }
   
   std::vector<std::pair<std::string, std::string>> bv_client_connection_manager::get_config_from_cluster(){ //  pull updated endpoints from cluster, by http request
      std::vector<std::pair<std::string, std::string>> cluster_config;
      int index = 0;
      #ifdef CONNECT_MGR_LOAD_BALANCE
      #ifdef CONNECT_MGR_LOAD_BALANCE_RANDOM
      #endif
      #endif
      return cluster_config;
   }
   
   bool bv_client_connection_manager::set_config(const std::vector<std::pair<std::string, std::string>> &  config){
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
      sptr<sync_response> ret;
      return ret;
   }
   
   bool bv_client_connection_manager::async_send(bv_client_connnection, sptr<sync_response> pframe, int frame_len){ // return after push frame to queue, no wait, will fail if queue full
      return true;
   }
   
   sptr<sync_response> bv_client_connection_manager::async_get_response(bv_client_connnection bvc_id){  // called by client's callback function
   }
   
   bv_client_connnection & bv_client_connection_manager::create_connection(){
   }
   bv_client_connnection & create_connection(std::string ip, std::string port){
      
   }
   bv_client_connnection & create_leader_connection(){
   }


}
}
