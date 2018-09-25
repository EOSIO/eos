#include <eosiolib/eosio.hpp>
#include <string>

using eosio::contract;
using eosio::print;
using std::string;

class regaction : public contract {
  public:
      using contract::contract;

      /// @abi action 
      void addaction( string receiver, string action, string tablename, string operation ) {
         print( receiver, " ", tablename, " ", action, " ", operation );
      }

      /// @abi action
      void delaction( string receiver, string action ) {
         print( receiver, " ", action );
      }

      /// @abi action
      void createindex( string tablename, string keys, string options ) {
         print( tablename, " ", keys, " ", options);
      }
};

EOSIO_ABI( regaction, (addaction)(delaction)(createindex) )
