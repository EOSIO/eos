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
      block_id_type result = fc::sha256::hash( static_cast<const block_header&>(*this) );
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

   checksum256_type signed_block_summary::calculate_transaction_mroot()const {
      vector<digest_type> merkle_of_each_shard;
      for(const region_summary& rs : regions) {
         for(const cycle& cs : rs.cycles_summary) {
            for(const shard_summary& ss: cs) {
               vector<digest_type> merkle_list_for_txns_in_shard;
               for(const transaction_receipt& tr : ss.transactions) {
                  merkle_list_for_txns_in_shard.emplace_back(tr.id);
               }
               merkle_of_each_shard.emplace_back( merkle(std::move(merkle_list_for_txns_in_shard)) );
            }
         }
      }
      return merkle( std::move(merkle_of_each_shard) );
    }

   void shard_trace::calculate_root() {
      static const size_t GUESS_ACTS_PER_TX = 10;

      vector<digest_type> action_roots;
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

} }
