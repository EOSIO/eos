#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

class hello : public eosio::contract {
public:
   using contract::contract;

   /// @abi action
   void hi( account_name user ) {
      print( "Hello, ", name{ user } );
   }

   /// @abi actoin
   void addword( string word, string desc, uint64_t id ) {
      print( word, "\t", desc, "\t", id );
   }

   /// @abi action
   void delword( string word ) {
      print( word );
   }

   /// @abi action
   void modword( string word, uint64_t id ) {
      print( word, "\t", id );
   }

   /// @abi action
   void testvec( vector<string> w, vector<uint64_t> v ) {

   }

};

EOSIO_ABI( hello, ( hi )( addword )( delword )( modword )( testvec ))
