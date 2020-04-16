#pragma once

#include <eosio/chain/block_state.hpp>
#include <eosio/state_history/types.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace eosio {
namespace state_history {

using chain::block_state_ptr;
using chain::transaction_id_type;
using chain::packed_transaction_ptr;
using compression_type = chain::packed_transaction::prunable_data_type::compression_type;

struct trace_converter {
   std::map<transaction_id_type, augmented_transaction_trace> cached_traces;
   fc::optional<augmented_transaction_trace>                  onblock_trace;
   
   void  add_transaction(const transaction_trace_ptr& trace, const packed_transaction_ptr& transaction);

   bytes pack(const chainbase::database& db, bool trace_debug_mode, const block_state_ptr& block_state,
              uint32_t version, compression_type compression);
   static bytes to_traces_bin_v0(const bytes& entry_payload, uint32_t version); 

   /**
    * Prune the signatures and context free data in a v1 log entry payload
    * 
    * @param[in, out]  entry_payload  The entry_payload 
    * @param[in] version              The version of the entry
    * @param[in, out] ids             The list of transaction ids to be pruned. After the member function returns, 
    *                                 it would be modified to contain the list of transaction ids that do not exists
    *                                 in the log entry.
    * 
    * @returns The offset to the input entry that has been modified
    **/
   static uint64_t prune_traces(bytes& entry_payload, uint32_t version, std::vector<transaction_id_type>& ids);

};

} // namespace state_history
} // namespace eosio
