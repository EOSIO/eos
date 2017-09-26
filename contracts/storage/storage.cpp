#include "storage.hpp"

namespace TOKEN_NAME {
   void storeAccount( AccountName name, const Account& account ) {
      if ( account.isEmpty() ) {
         Accounts::remove( account, name );
      } else {
         Accounts::store(account, name );
      } 
   }

   void apply_storage_transfer( const TOKEN_NAME::Transfer& transfer ) {
      eos::requireNotice( transfer.to, transfer.from );
      eos::requireAuth( transfer.from );

      Account from = getAccount( transfer.from );
      Account to   = getAccount( transfer.to );

      from.balance -= transfer.quantity; /// token subtraction has underflow assertion
      to.balance   += transfer.quantity; /// token addition has overflow assertion

      storeAccount( transfer.from, from );
      storeAccount( transfer.to, to );
   }

   bool validate_ipfspath( const char* ipfspath, uint32_t len ) {
      // To be implemented
      return true;  
   }

   bool validate_eospath( const char* eospath, uint32_t len ) {
      // To be implemented
      return true;
   }

   uint32_t readLinkFromBuffer( const char* buffer, uint32_t bufferlen, 
                                TOKEN_NAME::Link& link, uint32_t& eospathlen, uint32_t ipfspathlen ) {
      // To be implemented
      return 0;
   }
 
   void apply_storage_setlink() {
      TOKEN_NAME::Link link;
      uint32_t eospathlen;
      uint32_t ipfspathlen;
      char tmp[4098];
      auto bufferlen = readMessage(tmp, 4098);
      auto linklen = readLinkFromBuffer( tmp, bufferlen, link, eospathlen, ipfspathlen );
      eos::requireNotice( link.owner );
      eos::requireAuth( link.owner );
      validate_ipfspath( link.ipfspath, ipfspathlen );
      validate_eospath( link.eospath, eospathlen );
      ::store_str( currentCode(), N(storage), link.eospath, eospathlen, (char*)&link, linklen );
   }
   
   void apply_storage_removelink( char* eospath, uint32_t eospathlen ) {
      char tmp[4098];
      auto len = ::load_str( currentCode(), currentCode(), N(storage), eospath, eospathlen, tmp, 4098 );
      TOKEN_NAME::Link link;
      uint32_t ipfspathlen;
      len = readLinkFromBuffer( tmp, len, link, eospathlen, ipfspathlen );
      eos::requireAuth( link.owner );
      uint32_t stake = link.stake;
      ::remove_str( currentCode(), N(storage), link.eospath, eospathlen );
      // Reduce Quota usage in Account table
      // How does producer know to free cached file?
   }
   
   void apply_storage_createstore( char* eospath, uint32_t eospathlen ) {
      char tmp[4098];
      auto len = ::load_str( currentCode(), currentCode(), N(storage), eospath, eospathlen, tmp, 4098 );
      TOKEN_NAME::Link link;
      uint32_t ipfspathlen;
      len = readLinkFromBuffer( tmp, len, link, eospathlen, ipfspathlen );
      
      // eos::requireAuth( producer )
      // How do we validate the requireAuth() is a producer?
      // logic goes here to reduce number of tokens and increase quote used using bancor algorithm
      link.accept = 1;
      ::store_str( currentCode(), N(storage), link.eospath, eospathlen, (char*)&link, len );
   }
   
   void apply_storage_rejectstore( char* eospath, uint32_t eospathlen ) {
      char tmp[4098];
      auto len = ::load_str( currentCode(), currentCode(), N(storage), eospath, eospathlen, tmp, 4098 );
      TOKEN_NAME::Link link;
      uint32_t ipfspathlen;
      len = readLinkFromBuffer( tmp, len, link, eospathlen, ipfspathlen );
      // eos::requireAuth( producer )
      // How do we validate the requireAuth() is a producer?
      link.accept = 0;
      ::store_str( currentCode(), N(storage), link.eospath, eospathlen, (char*)&link, len );
   }
}  // namespace TOKEN_NAME

using namespace TOKEN_NAME;

extern "C" {
    void init()  {
       // How do we initialize the storage capacity? By how much here?
       Accounts::store( Account( StorageTokens(1000ll*1000ll*1000ll) ), N(storage) );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(storage) ) {
          if( action == N(transfer) ) {
               TOKEN_NAME::apply_storage_transfer( eos::currentMessage< TOKEN_NAME::Transfer >() );
          } else if (action == N(setlink) ) {
               TOKEN_NAME::apply_storage_setlink(); 
          } else if (action == N(removelink) ) {
               char tmp[1025];
               auto len = readMessage( tmp, 1025 );
               TOKEN_NAME::apply_storage_removelink( tmp, len );
          } else if (action == N(acceptstore) ) {
               char tmp[1025];
               auto len = readMessage( tmp, 1025 );
               TOKEN_NAME::apply_storage_createstore( tmp, len );
          } else if (action == N(rejectstore) ) {
               char tmp[1025];
               auto len = readMessage( tmp, 1025 );
               TOKEN_NAME::apply_storage_rejectstore( tmp, len );
          } else {
               assert(0, "unknown message");
          }
       } else {
           assert(0, "unknown code");
       }
    }
}
