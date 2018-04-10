/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/block_trace.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <algorithm>

namespace eosio { namespace chain {

   void shard_trace::finalize_shard() {
      {
         static const size_t GUESS_ACTS_PER_TX = 10;
         vector<digest_type> action_roots;
         cpu_usage = 0;
         action_roots.reserve(transaction_traces.size() * GUESS_ACTS_PER_TX);
         for (const auto& tx :transaction_traces) {
            for (const auto& at: tx.action_traces) {
               digest_type::encoder enc;
               uint64_t region_id   = tx.region_id;
               uint64_t cycle_index = tx.cycle_index;

               fc::raw::pack(enc, at.receiver);
               fc::raw::pack(enc, at.act.account);
               fc::raw::pack(enc, at.act.name);
               fc::raw::pack(enc, at.act.data);
               fc::raw::pack(enc, region_id);
               fc::raw::pack(enc, cycle_index);
               fc::raw::pack(enc, at.data_access);

               action_roots.emplace_back(enc.result());

               cpu_usage += at.cpu_usage;
            }
         }
         shard_action_root = merkle(action_roots);
      }

      {
         vector<digest_type> trx_roots;
         trx_roots.reserve(transaction_traces.size());
         for( uint64_t trx_index = 0, num_trxs = transaction_traces.size(); trx_index < num_trxs; ++trx_index ) {
            const auto& tx = transaction_traces[trx_index];
            digest_type::encoder enc;
            uint64_t region_id   = tx.region_id;
            uint64_t cycle_index = tx.cycle_index;
            uint64_t shard_index = tx.shard_index;
            fc::raw::pack( enc, region_id ); // Technically redundant since it is included in the trx header, but can still be useful here.
            fc::raw::pack( enc, cycle_index );
            fc::raw::pack( enc, shard_index );
            fc::raw::pack( enc, trx_index );
            fc::raw::pack( enc, *static_cast<const transaction_receipt*>(&tx) );
            if( tx.packed_trx_digest.valid() ) {
               fc::raw::pack( enc, *tx.packed_trx_digest );
            }
            trx_roots.emplace_back(enc.result());
         }
         shard_transaction_root = merkle(trx_roots);
      }

   }

   digest_type block_trace::calculate_action_merkle_root()const {
      vector<digest_type> merkle_root_of_each_region; // Extra level of indirection is to allow parallel computation of region merkle roots
      for(const auto& rt: region_traces ) {
         vector<digest_type> merkle_list_for_region;
         merkle_list_for_region.reserve(64);
         for(const auto& ct: rt.cycle_traces ) {
            for(const auto& st: ct.shard_traces) {
               merkle_list_for_region.emplace_back(st.shard_action_root);
            }
         }
         merkle_root_of_each_region.emplace_back( merkle(std::move(merkle_list_for_region)) );
      }
      return merkle( std::move(merkle_root_of_each_region) );
   }

   uint64_t block_trace::calculate_cpu_usage() const {
      uint64_t cpu_usage = 0;
      for(const auto& rt: region_traces ) {
         for(const auto& ct: rt.cycle_traces ) {
            for(const auto& st: ct.shard_traces) {
               cpu_usage += st.cpu_usage;
            }
         }
      }

      return cpu_usage;
   }

   digest_type block_trace::calculate_transaction_merkle_root()const {
      vector<digest_type> merkle_root_of_each_region; // Extra level of indirection is to allow parallel computation of region merkle roots
      for( const auto& rt : region_traces ) {
         vector<digest_type> merkle_list_for_region;
         for( const auto& ct : rt.cycle_traces ) {
            for( const auto& st: ct.shard_traces ) {
                  merkle_list_for_region.emplace_back(st.shard_transaction_root);
            }
         }
         merkle_root_of_each_region.emplace_back( merkle(std::move(merkle_list_for_region)) );
      }
      return merkle( std::move(merkle_root_of_each_region) );
    }

} }
