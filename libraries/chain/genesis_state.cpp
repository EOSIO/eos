#include <eosio/chain/genesis_state.hpp>

namespace eosio { namespace chain {

genesis_state::genesis_state() {
   initial_timestamp = fc::time_point::from_iso_string( "2018-06-01T12:00:00" );
   initial_key = fc::variant(eosio_root_key).as<public_key_type>();
}

chain::chain_id_type genesis_state::compute_chain_id() const {
   digest_type::encoder enc;
   //fc::raw::pack( enc, *this );
   fc::raw::pack( enc, initial_timestamp);
   fc::raw::pack( enc, initial_key );
   fc::raw::pack( enc, initial_configuration );
   // Only include initial_kv_configuration if it's different from the default so that
   // we don't inadvertently change the chain_id of existing chains.
   if( initial_kv_configuration != default_initial_kv_configuration ) {
      fc::raw::pack( enc, initial_kv_configuration );
   }
   return chain_id_type{enc.result()};
}

} } // namespace eosio::chain
