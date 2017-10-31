#pragma once
#include <eosio/blockchain/database.hpp>
#include <eosio/blockchain/transaction.hpp>
#include <mutex>

namespace eosio { namespace blockchain {
   using namespace boost::multi_index;

   /**
    *  @class transaction_cache
    *  @brief stores information about transactions until they are irreversibly confirmed
    *
    *  Access to this class is thread safe.
    */
   class transaction_cache 
   {
      public:
         meta_transaction_ptr add_transaction( meta_transaction_ptr ptr );
         meta_transaction_ptr find_transaction( transaction_id_type id )const;
         void                     remove_transaction( transaction_id_type id );
         void                     remove_expired( time_point_sec before_time );

      private:
         struct by_expiration;
         typedef boost::multi_index_container< meta_transaction_ptr,
                 indexed_by<
                    ordered_unique< tag<by_id>, member<meta_transaction, const transaction_id_type, &meta_transaction::id> >,
                    ordered_unique< tag<by_expiration>, member<meta_transaction, const time_point_sec, &meta_transaction::expiration> >
                 > > transaction_index_type;

         mutable std::mutex     _transactions_mutex;
         transaction_index_type _transactions;
   };

} } /// eosio::blockchain
