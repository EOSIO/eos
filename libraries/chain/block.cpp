/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/block.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <algorithm>

namespace eosio { namespace chain {
   digest_type block_header::digest()const
   {
      return digest_type::hash(*this);
   }

   uint32_t block_header::num_from_id(const block_id_type& id)
   {
      return fc::endian_reverse_u32(id._hash[0]);
   }

   block_id_type signed_block_header::id()const
   {
      block_id_type result = fc::sha256::hash(*this);
      result._hash[0] &= 0xffffffff00000000;
      result._hash[0] += fc::endian_reverse_u32(block_num()); // store the block num in the ID, 160 bits is plenty for the hash
      return result;
   }

   public_key_type signed_block_header::signee()const
   {
      return fc::crypto::public_key(producer_signature, digest(), true/*enforce canonical*/);
   }

   void signed_block_header::sign(const private_key_type& signer)
   {
      producer_signature = signer.sign(digest());
   }

   bool signed_block_header::validate_signee(const public_key_type& expected_signee)const
   {
      return signee() == expected_signee;
   }

   checksum_type signed_block_summary::calculate_transaction_mroot()const
   {
      return checksum_type();// TODO ::hash(merkle(ids));
   }

   digest_type   signed_block::calculate_transaction_merkle_root()const {
      vector<digest_type> ids; 
      ids.reserve(input_transactions.size());

      for( const auto& t : input_transactions ) 
         ids.emplace_back( t.id() );

      return merkle( std::move(ids) );
   }

   void shard_trace::calculate_root() {
      static const size_t GUESS_ACTS_PER_TX = 10;

      vector<digest_type> action_roots;
      action_roots.reserve(transaction_traces.size() * GUESS_ACTS_PER_TX);
      for (const auto& tx :transaction_traces) {
         for (const auto& at: tx.action_traces) {
            digest_type::encoder enc;

            fc::raw::pack(enc, at.receiver);
            fc::raw::pack(enc, at.act.scope);
            fc::raw::pack(enc, at.act.name);
            fc::raw::pack(enc, at.act.data);
            fc::raw::pack(enc, at.data_access);

            action_roots.emplace_back(enc.result());
         }
      }
      shard_root = merkle(action_roots);
   }

   void cycle_trace::calculate_root() {
      vector<digest_type> shard_roots;
      shard_roots.reserve(shard_traces.size());
      for (auto& s_trace :shard_traces) {
         shard_roots.emplace_back(s_trace.shard_root);
      }

      cycle_root = merkle(shard_roots);
   }

   void region_trace::calculate_root() {
      vector<digest_type> cycle_roots;
      cycle_roots.reserve(cycle_traces.size());
      for (uint32_t index = 0; index < cycle_traces.size(); index++) {
         const auto& c_trace = cycle_traces.at(index);

         digest_type::encoder enc;
         fc::raw::pack(enc, index);
         fc::raw::pack(enc, c_trace.cycle_root);
         cycle_roots.emplace_back(enc.result());
      }
      region_root = merkle(cycle_roots);
   }

   digest_type block_trace::calculate_action_merkle_root()const {
      vector<digest_type> region_roots;
      region_roots.reserve(region_traces.size());
      for (uint32_t index = 0; index < region_roots.size(); index++) {
         const auto& r_trace = region_traces.at(index);

         digest_type::encoder enc;
         fc::raw::pack(enc, index);
         fc::raw::pack(enc, r_trace.region_root);
         region_roots.emplace_back(enc.result());
      }
      return merkle(region_roots);
   }

} }
