#include <eosio/blockchain/transaction.hpp>

#include <fc/io/raw.hpp>

namespace eosio { namespace blockchain {

   digest_type transaction::digest()const {
      digest_type::encoder enc;
      fc::raw::pack( enc, *this );
      return enc.result();
   }

   transaction_id_type signed_transaction::id()const {
      digest_type::encoder enc;
      fc::raw::pack( enc, *this );
      return enc.result();
   }

   meta_transaction::meta_transaction( processed_transaction_ptr t)
   :trx(move(t)), id( trx->id() ), expiration(trx->expiration)
   {

   }

   meta_transaction::meta_transaction( const signed_transaction& t )
   :trx( make_shared<processed_transaction>( std::ref(t) ) ),
    id( t.id() ), expiration(t.expiration)
   {
   }

} } /// eosio::blockchain
