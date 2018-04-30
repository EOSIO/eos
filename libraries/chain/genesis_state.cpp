/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/genesis_state.hpp>

// these are required to serialize a genesis_state
#include <fc/smart_ref_impl.hpp>   // required for gcc in release mode

namespace eosio { namespace chain {


chain::chain_id_type genesis_state::compute_chain_id() const {
   return initial_chain_id;
}

} } // namespace eosio::chain
