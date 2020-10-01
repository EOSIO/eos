#include<string>
#include<vector>
#include<pair>
#include <mutex>
#include<thread>
#include<unordered_map>
#include "connect_mgr_config.hpp"

using namespace std;

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
   bv_client_connection_manager()=0;  
   bv_client_connection_manager(const string & config_file_path)=0;

   vector<pair<string,string>> get_init_config(const string& config_file_path)= 0;  // fetch init cluster from file
   vector<pair<string,string>> get_config() { 
      return m_cluster_config; 
   };
   vector<pair<string, string>> get_config_from_cluster()=0; //  pull updated endpoints from cluster, by http request
   bool set_config(const vector<pair<string, string>>)=0;   

   vector<pair<string,string>> m_cluster_config;  // first one is leader m_cluster_config[0]
   string m_config_file_path; 
   sptr<sync_response>  sync_send(bv_client_connnection, sptr<sync_frame>  pframe, int frame_len)=0; // sync call wait until server response
   bool async_send(bv_client_connnection, sptr<sync_frame> pframe, int frame_len)=0; // return after push frame to queue, no wait, will fail if queue full
   sptr<sync_response>  async_get_response(bv_client_connnection bvc_id)=0;  // called by client's callback function

   // used only when call async_sync_frame_to_leader
   unordered_map<bv_client_connnection, queue<sptr<sync_frame>>> m_sync_que;    //   client manager will free the frame memory after frame sent.
   unordered_map<bv_client_connnection, queue<sptr<sync_response>>> m_response_que;    // nodeos caller will free response_frame memory after caller used it. 
   #ifdef CONNECT_MGR_THREAD_SAFE
   std::mutex m_config_mutex;
   #endif
   static sptr<bv_client_connection_base> connect_mgr_instance;
   
public:
   virtual bv_client_connnection & create_connection()=0;
   virtual bv_client_connnection & create_connection(string ip, string port)=0;
   virtual bv_client_connnection & create_leader_connection()=0;

   virtual delete_connection(bv_client_connnection ccid)=0;
   virtual ~bv_client_connection_manager()=0;
};

class bv_client_connection_manager : public bv_client_connection_base{
private:
   bv_client_connection_manager();  
   bv_client_connection_manager(const string & config_file_path);
   vector<pair<string,string>> get_init_config(const string& config_file_path);  // fetch init cluster from file
   vector<pair<string, string>> get_config_from_cluster(); //  pull updated endpoints from cluster, by http request

   sptr<sync_response>  sync_send(bv_client_connnection, sptr<sync_response>  pframe, int frame_len); // sync call wait until server response
   bool async_send(bv_client_connnection, sptr<sync_response>  pframe, int frame_len); // return after push frame to queue, no wait, will fail if queue full
   sptr<sync_response>  async_get_response(bv_client_connnection bvc_id);  // called by client's callback function

   bool set_config(const vector<pair<string, string>>);      
   
public:

   static  sptr<bv_client_connection_base> get_bvcc_manager_instance();
   static  sptr<bv_client_connection_base> get_bvcc_manager_instance(const string & config_file_path);

   virtual bv_client_connnection & create_connection();
   virtual bv_client_connnection & create_connection(string ip, string port);
   virtual bv_client_connnection & create_leader_connection();

   virtual bool delete_connection(bv_client_connnection bvcc_id);// return false if queue is not empty
   //virtual void sync_for_construction(bv_sync_handler& handler);
   virtual ~bv_client_connection_manager();

};

}

}
