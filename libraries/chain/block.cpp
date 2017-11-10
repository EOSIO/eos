/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/block.hpp>
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

   digest_type merkle(vector<digest_type> ids) {
      while (ids.size() > 1) {
         if (ids.size() % 2)
            ids.push_back(ids.back());
         for (int i = 0; i < ids.size() / 2; ++i)
            ids[i/2] = digest_type::hash(std::make_pair(ids[i], ids[i+1]));
         ids.resize(ids.size() / 2);
      }

      return ids.front();
   }

   checksum_type signed_block_summary::calculate_transaction_mroot()const
   {
      return checksum_type();// TODO ::hash(merkle(ids));
   }

} }
