#pragma once

#include<string>
#include<vector>
#include<queue>
#include<unordered_map>
#include<utility>
#include<mutex>
#include<thread>
#include "connect_mgr_config.hpp"

namespace eosio {

namespace blockvault_client{

struct sync_frame{
   uint32_t type;
   uint64_t size;
   uint64_t resume_token;
   uint8_t value[0];
}__attribute__ ((aligned (ALIGN_BYTES)));

using  sync_response = sync_frame;
using bv_client_connnection = int;

template<typename T>
using sptr = std::shared_ptr<T>;

template<typename T>
using uptr = std::unique_ptr<T>;

class bv_client_connection_base {
private:
   virtual std::vector<std::pair<std::string,std::string>> get_init_config(const std::string& config_file_path)= 0;  // fetch init cluster from file
   virtual std::vector<std::pair<std::string,std::string>> get_config() = 0;
   virtual std::vector<std::pair<std::string, std::string>> get_config_from_cluster()=0; //  pull updated endpoints from cluster, by http request
   virtual bool set_config(const std::vector<std::pair<std::string, std::string>>  & config)=0;   

   virtual sptr<sync_response>  sync_send(bv_client_connnection, sptr<sync_frame>  pframe, int frame_len)=0; // sync call wait until server response
   virtual bool async_send(bv_client_connnection, sptr<sync_frame> pframe, int frame_len)=0; // return after push frame to queue, no wait, will fail if queue full
   virtual sptr<sync_response>  async_get_response(bv_client_connnection bvc_id)=0;  // called by client's callback function
   
public:
   bv_client_connection_base();  
   bv_client_connection_base(const std::string & config_file_path);
   virtual ~bv_client_connection_base();
   virtual bv_client_connnection & create_connection()=0;
   virtual bv_client_connnection & create_connection(std::string ip, std::string port)=0;
   virtual bv_client_connnection & create_leader_connection()=0;
   virtual bool delete_connection(bv_client_connnection ccid)=0;
};

class bv_client_connection_manager : public bv_client_connection_base{
private:
   bv_client_connection_manager();  
   bv_client_connection_manager(const std::string & config_file_path);

   std::vector<std::pair<std::string, std::string>> get_init_config(const std::string& config_file_path);  // fetch init cluster from file
   std::vector<std::pair<std::string, std::string>> get_config_from_cluster(); //  pull updated endpoints from cluster, by http request
   virtual std::vector<std::pair<std::string,std::string>> get_config(){
      return m_cluster_config;
   }
   sptr<sync_response>  sync_send(bv_client_connnection, sptr<sync_response>  pframe, int frame_len); // sync call wait until server response
   bool async_send(bv_client_connnection, sptr<sync_response>  pframe, int frame_len); // return after push frame to queue, no wait, will fail if queue full
   sptr<sync_response>  async_get_response(bv_client_connnection bvc_id);  // called by client's callback function

   virtual bool set_config(const std::vector<std::pair<std::string, std::string>> & config);      
   std::unordered_map<bv_client_connnection, std::queue<sptr<sync_frame> > >  m_sync_que_map;    
   std::unordered_map<bv_client_connnection, std::queue<sptr<sync_response> > >  m_response_que_map;    
   #ifdef CONNECT_MGR_THREAD_SAFE
   std::mutex m_config_mutex;
   #endif
   std::string m_config_file_path; 
   std::vector<std::pair<std::string,std::string>> m_cluster_config;  // first one is leader m_cluster_config[0]
   
public:
   static  sptr<bv_client_connection_manager> get_bvcc_manager_instance();
   static  sptr<bv_client_connection_manager> get_bvcc_manager_instance(const std::string & config_file_path);

   virtual bv_client_connnection & create_connection();
   virtual bv_client_connnection & create_connection(std::string ip, std::string port);
   virtual bv_client_connnection & create_leader_connection();

   virtual bool delete_connection(bv_client_connnection bvcc_id);// return false if queue is not empty
   static sptr<bv_client_connection_manager> connect_mgr_instance;
   virtual ~bv_client_connection_manager();
   //virtual void sync_for_construction(bv_sync_handler& handler);
};

}

}
