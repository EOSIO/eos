#include <eosiolib/eosio.hpp>
#include <string>

using eosio::contract;
using eosio::print;
using std::string;

class regaction : public contract {
public:
   using contract::contract;

   /// @abi action
   void addaction( string receiver, string action, string collection, int32_t operation, int32_t idxnum ) {
      print( receiver, " ", action, " ", collection, " ", operation, " ", idxnum );
   }

   /// @abi action
   void reginsact( string receiver, string action, string collection ) {
      SEND_INLINE_ACTION( *this, addaction, { _self, N( active ) }, { receiver, action, collection, 1, 1 } );
   }

   /// @abi action
   void regmodact( string receiver, string action, string collection, int32_t idxnum ) {
      SEND_INLINE_ACTION( *this, addaction, { _self, N( active ) }, { receiver, action, collection, 2, idxnum } );
   }

   /// @abi action
   void regdelact( string receiver, string action, string collection, int32_t idxnum ) {
      SEND_INLINE_ACTION( *this, addaction, { _self, N( active ) }, { receiver, action, collection, 3, idxnum } );
   }

   /// @abi action
   void delaction( string receiver, string action ) {
      print( receiver, " ", action );
   }

   /// @abi action
   void createindex( string collection, string keys, string options ) {
      print( collection, " ", keys, " ", options );
   }
};

EOSIO_ABI( regaction, ( addaction )( delaction )( createindex )( reginsact )( regmodact )( regdelact ))
