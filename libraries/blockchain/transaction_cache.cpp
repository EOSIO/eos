#include <eosio/blockchain/transaction_cache.hpp>

namespace eosio { namespace blockchain {

   meta_transaction_ptr  transaction_cache::add_transaction( meta_transaction_ptr trx ) {
      std::unique_lock<std::mutex> lock(_transactions_mutex);

      auto itr = _transactions.find( trx->id );
      if( itr == _transactions.end() ) {
         _transactions.insert( trx );
         return trx;
      }
      return *itr;
   }

   meta_transaction_ptr  transaction_cache::find_transaction( transaction_id_type id )const {
      std::unique_lock<std::mutex> lock(_transactions_mutex);

      auto itr = _transactions.find( id );
      if( itr == _transactions.end() ) {
         return meta_transaction_ptr();
      }
      return *itr;
   }


} } /// eosio::blockchain 
