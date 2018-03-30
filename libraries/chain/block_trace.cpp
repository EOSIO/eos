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
      static const size_t GUESS_ACTS_PER_TX = 10;

      vector<digest_type> action_roots;
      cpu_usage = 0;
      action_roots.reserve(transaction_traces.size() * GUESS_ACTS_PER_TX);
      for (const auto& tx :transaction_traces) {
         for (const auto& at: tx.action_traces) {
            digest_type::encoder enc;

            fc::raw::pack(enc, at.receiver);
            fc::raw::pack(enc, at.act.account);
            fc::raw::pack(enc, at.act.name);
            fc::raw::pack(enc, at.act.data);
            fc::raw::pack(enc, at.region_id);
            fc::raw::pack(enc, at.cycle_index);
            fc::raw::pack(enc, at.data_access);

            action_roots.emplace_back(enc.result());

            cpu_usage += at.cpu_usage;
         }
      }
      shard_root = merkle(action_roots);
   }

   digest_type block_trace::calculate_action_merkle_root()const {
      vector<digest_type> shard_roots;
      shard_roots.reserve(1024);
      for(const auto& rt: region_traces ) {
         for(const auto& ct: rt.cycle_traces ) {
            for(const auto& st: ct.shard_traces) {
               shard_roots.emplace_back(st.shard_root);
            }
         }
      }
      return merkle(shard_roots);
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

} }
