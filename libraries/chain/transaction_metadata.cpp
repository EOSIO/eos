#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace chain {

transaction_metadata::transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published, const optional<time_point>& processing_deadline, bool implicit )
   :raw_trx(t.get_raw_transaction())
   ,decompressed_trx(fc::raw::unpack<transaction>(*raw_trx))
   ,context_free_data(t.get_context_free_data())
   ,id(decompressed_trx->id())
   ,billable_packed_size( t.get_billable_size() )
   ,signature_count(t.signatures.size())
   ,published(published)
   ,packed_digest(t.packed_digest())
   ,raw_data(raw_trx->data())
   ,raw_size(raw_trx->size())
   ,is_implicit(implicit)
   ,processing_deadline(processing_deadline)
{ }

} } // eosio::chain
