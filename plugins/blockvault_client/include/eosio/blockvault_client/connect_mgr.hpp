#include<string>
#include<vector>
#include<pair>
#include "connect_mgr_config.hpp"
#ifdef THREAD_SAFE
#include <mutex>
#endif
#include<thread>

using namespace std;

struct sync_frame{
   uint32_t type;
   uint64_t size;
   uint64_t resume_token;
   uint8_t value[0];
}__attribute__ ((aligned (ALIGN_BYTES)));

using  sync_response = sync_frame;
using bv_client_connnection = int;
class bv_client_connection_base {
private:
   vector<pair<string,string>> get_init_config(const string& config_file_path)= 0;  // fetch init cluster from file
   vector<pair<string,string>> get_config() { return m_cluster_config; };
   vector<pair<string, string>> get_config_from_cluster()=0; //  pull updated endpoints from cluster, by http request

   bool set_config(const vector<pair<string, string>>)=0;   

   vector<pair<string,string>> m_cluster_config;
   string m_config_file_path; 
   // used only when call async_sync_frame_to_leader
   vector<queue<sync_frame *>> m_sync_que;    //   client manager will free the frame memory after frame sent.
   vector<queue<sync_response*>> m_response_que;    // nodeos caller will free response_frame memory after caller used it. 
   
public:
   bv_client_connection_manager()=0;  
   bv_client_connection_manager(const string & config_file_path)=0;
   sync_response * sync_frame_to_leader(bv_client_connnection, sync_frame * pframe, int frame_len)=0; // sync call wait until server response
   bool async_sync_frame_to_leader(bv_client_connnection, sync_frame * pframe, int frame_len)=0; // return after push frame to queue, no wait, will fail if queue full
   sync_response * async_get_frame_response(bv_client_connnection bvc_id)=0;  // called by client's callback function
   virtual bv_client_connnection & create_connection()=0;
   virtual ~bv_client_connection_manager()=0;
};

class bv_client_connection_manager{
private:
   vector<pair<string,string>> get_init_config(const string& config_file_path);  // fetch init cluster from file
   vector<pair<string, string>> get_config_from_cluster(); //  pull updated endpoints from cluster, by http request

   bool set_config(const vector<pair<string, string>>);      
public:
   bv_client_connection_manager();  
   bv_client_connection_manager(const string & config_file_path);
   sync_response * sync_frame_to_leader(bv_client_connnection, sync_frame * pframe, int frame_len); // sync call wait until server response
   bool async_sync_frame_to_leader(bv_client_connnection, sync_frame * pframe, int frame_len); // return after push frame to queue, no wait, will fail if queue full
   sync_response * async_get_frame_response(bv_client_connnection bvc_id);  // called by client's callback function

   virtual bv_client_connnection & create_connection();
   virtual ~bv_client_connection_manager();
};
