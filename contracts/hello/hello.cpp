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
      SEND_INLINE_ACTION( *this, bye, { _self, N( active ) }, { user } );
      print( "Hello, ", name{ user } );
      SEND_INLINE_ACTION( *this, bye2, { _self, N( active ) }, { user } );
   }

   void bye( account_name user ) {
      print( "bye, ", name{ user } );
   }

   void bye2( account_name user ) {
      print( "bye2, ", name{ user } );
   }

   /// @abi actoin
   void addword( string word, string desc, uint64_t id ) {
   }

   /// @abi action
   void delword( string word ) {
   }

   /// @abi action
   void modword( string word, uint64_t id ) {
   }

   /// @abi action
   void testvec( vector<string> w, vector<uint64_t> v ) {

   }

   /// @abi actoin
   void addint( uint64_t id, string val ) {

   }

   /// @abi action
   void modint( uint64_t id, string val ) {

   }

   /// @abi action
   void delint( uint64_t id ) {

   }

};

EOSIO_ABI( hello, ( hi )( addword )( delword )( modword )( testvec )( addint )( modint )( delint )( bye )( bye2 ))
