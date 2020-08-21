#include <eosio/eosio.hpp>                   /* contract, datastream, check */
#include <eosio/privileged.hpp>              /* blockchain_parameters */
#include <vector>                            /* vector */
using namespace eosio;
using namespace std;

#define STR_I(x) #x
#define STR(x) STR_I(x)
#define ASSERT_EQ(e1,e2) check((e1) == (e2), STR(__FILE__)STR(:)STR(__LINE__)": " #e1" != "#e2)
#define ASSERT_LESS(e1,e2) check((e1) < (e2), STR(__FILE__)STR(:)STR(__LINE__)": " #e1" < "#e2)
#define ASSERT_LE(e1,e2) check((e1) <= (e2), STR(__FILE__)STR(:)STR(__LINE__)": " #e1" <= "#e2)

#define IMPORT extern "C" __attribute__((eosio_wasm_import))

IMPORT uint32_t get_parameters_packed( const char* ids, uint32_t ids_size, char* params, uint32_t params_size);
IMPORT     void set_parameters_packed( const char* params, uint32_t params_size );

unsigned_int operator "" _ui(unsigned long long v){ return unsigned_int(v); }

uint16_t operator "" _16(unsigned long long v){ return static_cast<uint16_t>(v); }
uint32_t operator "" _32(unsigned long long v){ return static_cast<uint32_t>(v); }
uint64_t operator "" _64(unsigned long long v){ return static_cast<uint64_t>(v); }

struct params_object{
   string packed;
   template<typename T>
   params_object(T v){
      (*this)(v);
   }
   params_object(const string& params) : packed(params) {}

   bool operator == (const params_object& pp) const{
      return packed == pp.packed;
   }
   params_object& operator () (unsigned_int val){
      char buffer[4];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      ds << val;
      ASSERT_LE(ds.tellp(), sizeof(buffer));

      packed += {buffer, ds.tellp()};
      return *this;
   }
   params_object& operator () (uint16_t val){
      char buffer[2];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      ds << val;
      ASSERT_EQ(ds.tellp(), sizeof(buffer));

      packed += {buffer, ds.tellp()};
      return *this;
   }
   params_object& operator () (uint32_t val){
      char buffer[4];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      ds << val;
      ASSERT_EQ(ds.tellp(), sizeof(buffer));

      packed += {buffer, ds.tellp()};
      return *this;
   }
   params_object& operator () (uint64_t val){
      char buffer[8];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      ds << val;
      ASSERT_EQ(ds.tellp(), sizeof(buffer));

      packed += {buffer, ds.tellp()};
      return *this;
   }
   params_object& operator () (vector<unsigned_int>&& val){
      char buffer[512];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      ds << val;
      ASSERT_LESS(ds.tellp(), sizeof(buffer));

      packed += {buffer, ds.tellp()};
      return *this;
   }

   void set() const{
      set_parameters_packed(packed.c_str(), packed.size());
   }
   params_object get() const{
      char buffer[512];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));

      auto size = get_parameters_packed(packed.c_str(), packed.size(), buffer, sizeof(buffer));
      ASSERT_LESS(size, sizeof(buffer));
      return params_object{{buffer, size}};
   }
   size_t get_size() const{
      return get_parameters_packed(packed.c_str(), packed.size(), 0, 0);
   }
};

class [[eosio::contract("params_test")]] params_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]] void maintest(){

      //make sure no throw for zero parameters provided
      params_object(0_ui).set();
      ASSERT_EQ(params_object(0_ui).get(),
                params_object(0_ui));

      //1 for size, 1 for id, 8 for data
      ASSERT_EQ(params_object({1_ui, 0_ui}).get_size(), 10);
      //1 for size, 1 for id, 4 for data
      ASSERT_EQ(params_object({1_ui, 1_ui}).get_size(), 6);
      //1 for size, 1 for id, 2 for data
      ASSERT_EQ(params_object({1_ui, 16_ui}).get_size(), 4);
      //1 for size, 3 for ids, 8+4+2 for data
      ASSERT_EQ(params_object({3_ui, 0_ui, 1_ui, 16_ui}).get_size(), 18);

      blockchain_parameters old_way_params;
      params_object(1_ui)(0_ui)(1024*1024*2_64).set();
      get_blockchain_parameters(old_way_params);
      ASSERT_EQ(params_object({1_ui,0_ui}).get(), 
                params_object(1_ui)(0_ui)(1024*1024*2_64));
      ASSERT_EQ(old_way_params.max_block_net_usage, 1024*1024*2);

      params_object(1_ui)(1_ui)(20*100_32).set();
      get_blockchain_parameters(old_way_params);
      ASSERT_EQ(params_object({1_ui,1_ui}).get(), 
                params_object(1_ui)(1_ui)(20*100_32));
      ASSERT_EQ(old_way_params.target_block_net_usage_pct, 20*100);
      
      params_object(1_ui)(16_ui)(8_16).set();
      get_blockchain_parameters(old_way_params);
      ASSERT_EQ(params_object({1_ui,16_ui}).get(), 
                params_object(1_ui)(16_ui)(8_16));
      ASSERT_EQ(old_way_params.max_authority_depth, 8);

      params_object(3_ui)(0_ui)(1024*1024*3_64)
                         (1_ui)(30*100_32)
                         (16_ui)(10_16).set();
      get_blockchain_parameters(old_way_params);
      ASSERT_EQ(params_object({3_ui, 0_ui, 1_ui, 16_ui}).get(), 
                params_object(3_ui)(0_ui)(1024*1024*3_64)
                                   (1_ui)(30*100_32)
                                   (16_ui)(10_16));
      ASSERT_EQ(old_way_params.max_block_net_usage, 1024*1024*3);
      ASSERT_EQ(old_way_params.target_block_net_usage_pct, 30*100);
      ASSERT_EQ(old_way_params.max_authority_depth, 10);

      params_object(17_ui) (0_ui)(1024*1024*4_64)
                           (1_ui)(35*100_32)
                           (2_ui)(1024*1024*2_32)
                           (3_ui)(1024_32)
                           (4_ui)(700_32)
                           (5_ui)(50_32)
                           (6_ui)(120_32)
                           (7_ui)(250'000_32)
                           (8_ui)(15*100_32)
                           (9_ui)(230'000_32)
                           (10_ui)(1000_32)
                           (11_ui)(30*60_32)
                           (12_ui)(15*60_32)
                           (13_ui)(30*24*3600_32)
                           (14_ui)(1024*1024_32)
                           (15_ui)(6_16)
                           (16_ui)(9_16).set();
      get_blockchain_parameters(old_way_params);
      ASSERT_EQ(params_object({17_ui,0_ui,1_ui,2_ui,3_ui,4_ui,
                                     5_ui,6_ui,7_ui,8_ui,9_ui,
                                     10_ui,11_ui,12_ui,13_ui,
                                     14_ui,15_ui,16_ui}).get(), 
                params_object(17_ui)(0_ui)(1024*1024*4_64)
                                    (1_ui)(35*100_32)
                                    (2_ui)(1024*1024*2_32)
                                    (3_ui)(1024_32)
                                    (4_ui)(700_32)
                                    (5_ui)(50_32)
                                    (6_ui)(120_32)
                                    (7_ui)(250'000_32)
                                    (8_ui)(15*100_32)
                                    (9_ui)(230'000_32)
                                    (10_ui)(1000_32)
                                    (11_ui)(30*60_32)
                                    (12_ui)(15*60_32)
                                    (13_ui)(30*24*3600_32)
                                    (14_ui)(1024*1024_32)
                                    (15_ui)(6_16)
                                    (16_ui)(9_16));
      ASSERT_EQ(old_way_params.max_block_net_usage, 1024*1024*4);
      ASSERT_EQ(old_way_params.target_block_net_usage_pct, 35*100);
      ASSERT_EQ(old_way_params.max_transaction_net_usage, 1024*1024*2);
      ASSERT_EQ(old_way_params.base_per_transaction_net_usage, 1024);
      ASSERT_EQ(old_way_params.net_usage_leeway, 700);
      ASSERT_EQ(old_way_params.context_free_discount_net_usage_num, 50);
      ASSERT_EQ(old_way_params.context_free_discount_net_usage_den, 120);
      ASSERT_EQ(old_way_params.max_block_cpu_usage, 250'000);
      ASSERT_EQ(old_way_params.target_block_cpu_usage_pct, 15*100);
      ASSERT_EQ(old_way_params.max_transaction_cpu_usage, 230'000);
      ASSERT_EQ(old_way_params.min_transaction_cpu_usage, 1000);
      ASSERT_EQ(old_way_params.max_transaction_lifetime, 30*60);
      ASSERT_EQ(old_way_params.deferred_trx_expiration_window, 15*60);
      ASSERT_EQ(old_way_params.max_transaction_delay, 30*24*3600);
      ASSERT_EQ(old_way_params.max_inline_action_size, 1024*1024);
      ASSERT_EQ(old_way_params.max_inline_action_depth, 6);
      ASSERT_EQ(old_way_params.max_authority_depth, 9);

      //v1 config, max_action_return_value_size
      ASSERT_EQ(params_object(1_ui)(17_ui).get(), 
                params_object(1_ui)(17_ui)(256_32));
      
      params_object(1_ui)(17_ui)(512_32).set();
      ASSERT_EQ(params_object(1_ui)(17_ui).get(), 
                params_object(1_ui)(17_ui)(512_32));
   }

   [[eosio::action]] void setthrow1(){
      //unknown configuration index
      params_object(1_ui)(100_ui).set();    
   }

   [[eosio::action]] void setthrow2(){
      //length=2, only 1 argument provided
      params_object(2_ui)(1_ui).set();    
   }

   [[eosio::action]] void setthrow3(){
      //passing argument that will fail validation
      params_object(1_ui)(1_ui)(200*100_32).set();
   }

   [[eosio::action]] void getthrow1(){
      //unknown configuration index
      params_object(1_ui)(100_ui).get();    
   }

   [[eosio::action]] void getthrow2(){
      //length=2, only 1 argument provided
      params_object(2_ui)(1_ui).get();    
   }

   [[eosio::action]] void getthrow3(){
      //buffer too small
      char buffer[4];
      datastream<char*> ds((char*)&buffer, sizeof(buffer));
      auto pp = params_object(2_ui)(0_ui)(1_ui);
      get_parameters_packed(pp.packed.c_str(), pp.packed.size(), buffer, sizeof(buffer));
   }

   [[eosio::action]] void throwrvia1(){
      //throws when setting parameter that is not allowed because of protocol feature for
      //this parameter is not active
      
      //v1 config, max_action_return_value_size
      params_object(1_ui)(17_ui)(1024_32).set();
      ASSERT_EQ(params_object(1_ui)(17_ui).get(), 
                params_object(1_ui)(17_ui)(1024_32));
   }

   [[eosio::action]] void throwrvia2(){
      //this test tries to get parameter with corresponding inactive protocol feature
      
      //v1 config, max_action_return_value_size
      ASSERT_EQ(params_object(1_ui)(17_ui).get(), 
                params_object(1_ui)(17_ui)(1024_32));
   }
};