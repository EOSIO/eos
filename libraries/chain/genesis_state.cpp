/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosio/chain/genesis_state.hpp>

// these are required to serialize a genesis_state
#include <fc/smart_ref_impl.hpp>   // required for gcc in release mode

namespace eosio { namespace chain {

genesis_state::genesis_state() {
   initial_timestamp = fc::time_point::from_iso_string( "2018-06-01T12:00:00" );
   initial_key = fc::variant(eosio_root_key).as<public_key_type>();

   initial_configuration.max_block_usage =
      std::vector<uint64_t>(config::default_max_block_usage.begin(), config::default_max_block_usage.end());
   initial_configuration.max_transaction_usage =
      std::vector<uint64_t>(config::default_max_transaction_usage.begin(), config::default_max_transaction_usage.end());
   initial_configuration.target_virtual_limits =
      std::vector<uint64_t>(config::default_target_virtual_limits.begin(), config::default_target_virtual_limits.end());
   initial_configuration.min_virtual_limits =
      std::vector<uint64_t>(config::default_min_virtual_limits.begin(), config::default_min_virtual_limits.end());
   initial_configuration.max_virtual_limits =
      std::vector<uint64_t>(config::default_max_virtual_limits.begin(), config::default_max_virtual_limits.end());
   initial_configuration.usage_windows =
      std::vector<uint32_t>(config::default_usage_windows.begin(), config::default_usage_windows.end());
   initial_configuration.virtual_limit_decrease_pct =
      std::vector<uint16_t>(config::default_virtual_limit_decrease_pct.begin(), config::default_virtual_limit_decrease_pct.end());
   initial_configuration.virtual_limit_increase_pct =
      std::vector<uint16_t>(config::default_virtual_limit_increase_pct.begin(), config::default_virtual_limit_increase_pct.end());
   initial_configuration.account_usage_windows =
      std::vector<uint32_t>(config::default_account_usage_windows.begin(), config::default_account_usage_windows.end());
}

chain::chain_id_type genesis_state::compute_chain_id() const {
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return chain_id_type{enc.result()};
}

} } // namespace eosio::chain
