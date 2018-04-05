#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace chain {

transaction_metadata::transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published, bool implicit )
   :raw_trx(t.get_raw_transaction())
   ,decompressed_trx(fc::raw::unpack<transaction>(*raw_trx))
   ,context_free_data(t.get_context_free_data())
   ,id(decompressed_trx->id())
   ,billable_packed_size( t.get_billable_size() )
   ,signature_count(t.signatures.size())
   ,published(published)
   ,raw_data(raw_trx->data())
   ,raw_size(raw_trx->size())
   ,is_implicit(implicit)
{ }

digest_type transaction_metadata::calculate_transaction_merkle_root( const vector<transaction_metadata>& metas ) {
   vector<digest_type> ids;
   ids.reserve(metas.size());

   for( const auto& t : metas ) {
      ids.emplace_back(t.id);
   }

   return merkle( std::move(ids) );
}

} } // eosio::chain
