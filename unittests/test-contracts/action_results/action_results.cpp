#include <eosio/eosio.hpp>                   /* contract, datastream, unsigned_int */
#include <vector>                            /* vector */

using namespace eosio;
using namespace std;

extern "C" { 
__attribute__((eosio_wasm_import))
void set_parameters_packed( const char* params, uint32_t params_size );
__attribute__((eosio_wasm_import))
void set_action_return_value(void*, size_t);
};

class [[eosio::contract]] action_results : public contract {
  public:
      using contract::contract;

      [[eosio::action]]
      int actionresret() {
         return 10;
      }

      [[eosio::action]]
      vector<char> retoverlim() {
         return vector<char>(512, '0');
      }

      [[eosio::action]]
      vector<char> retlim() {
         //2 is for size of type unsigned_int
         return vector<char>(256 - 2, '0');
      }

      [[eosio::action]]
      vector<char> ret1overlim() {
         //2 is for size of type unsigned_int
         return vector<char>(257 - 2, '0');
      }

      /**
       * required to be called as system contract for priviledged host function call  
       */
      [[eosio::action]]
      void retmaxlim() {
         
         char buffer[12];
         datastream<char*> ds((char*)&buffer, sizeof(buffer));
         //20mb is MAX_SIZE_OF_BYTE_ARRAYS that is defined in fc.
         //we don't use fc in contracts so using hardcode here.
         ds << unsigned_int(uint32_t(1)) << unsigned_int(uint32_t(17)) << uint32_t(20*1024*1024);
         set_parameters_packed(buffer, ds.tellp());

         vector<char> ret_vec(20*1024*1024, '1');
         //return ret_vec;
         set_action_return_value( ret_vec.data(), ret_vec.size() );
      }

      [[eosio::action]]
      void setliminv() {
         
         char buffer[12];
         datastream<char*> ds((char*)&buffer, sizeof(buffer));
         
         ds << unsigned_int(uint32_t(1)) << unsigned_int(uint32_t(17)) << uint32_t(20*1024*1024 + 1);
         //trying to set limit too large
         set_parameters_packed(buffer, ds.tellp());
      }

  private:
};
