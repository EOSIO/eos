#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_key_value_any_lookup.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/combined_database.hpp>

namespace eosio { namespace chain { namespace backing_store {

   key_bundle::key_bundle(const b1::chain_kv::bytes& composite_key, name code)
   : full_key(db_key_value_format::create_full_key(composite_key, code)){
   }

   prefix_bundle::prefix_bundle(const b1::chain_kv::bytes& composite_key, end_of_prefix prefix_end, name code)
   : full_key(db_key_value_format::create_full_key(composite_key, code)),
     prefix_key(db_key_value_format::create_full_key_prefix(full_key, prefix_end)) {
   }

}}} // namespace eosio::chain::backing_store
