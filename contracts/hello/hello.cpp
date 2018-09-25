#include <eosiolib/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

class hello : public eosio::contract {
public:
   using contract::contract;

   /// @abi action
   void hi(account_name user) {
      print("Hello, ", name{user});
   }

   /// @abi actoin
   void addword(string document) {
      print(document);
   }

   /// @abi action
   void delword( string filter ) {

   }

   /// @abi action
   void modword( string filter, string update ) {

   }

};

EOSIO_ABI( hello, (hi)(addword)(delword)(modword) )
