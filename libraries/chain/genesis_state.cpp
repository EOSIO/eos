#include <eosio/chain/genesis_state.hpp>

namespace eosio { namespace chain {

genesis_state::genesis_state() {
   initial_timestamp = fc::time_point::from_iso_string( "2018-06-01T12:00:00" );
   initial_key = fc::variant(eosio_root_key).as<public_key_type>();
}

chain::chain_id_type genesis_state::compute_chain_id() const {
   digest_type::encoder enc;

   // These fields were in the original genesis state and should just be encoded directly.
   fc::raw::pack( enc, initial_timestamp );
   fc::raw::pack( enc, initial_key );
   fc::raw::pack( enc, initial_configuration );

   // If initial_protocol_features is empty, it should not be included in the digest.
   for(const digest_type& feature_digest : initial_protocol_features) {
      fc::raw::pack( enc, feature_digest );
   }

   return chain_id_type{enc.result()};
}

} } // namespace eosio::chain
