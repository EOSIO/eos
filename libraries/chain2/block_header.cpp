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
      // Do not include signed_block_header attributes in id, specifically exclude producer_signature.
      block_id_type result = fc::sha256::hash(*static_cast<const block_header*>(this));
      result._hash[0] &= 0xffffffff00000000;
      result._hash[0] += fc::endian_reverse_u32(block_num()); // store the block num in the ID, 160 bits is plenty for the hash
      return result;
   }

   public_key_type signed_block_header::signee( const digest_type& schedule_digest )const
   {
      return fc::crypto::public_key(producer_signature, signed_digest( schedule_digest ), true/*enforce canonical*/);
   }

   digest_type signed_block_header::signed_digest( const digest_type& schedule_digest )const {
      digest_type::encoder enc;
      fc::raw::pack( enc, schedule_digest );
      fc::raw::pack( enc, digest() );
      return enc.result();
   }

   void signed_block_header::sign(const private_key_type& signer, const fc::sha256& schedule_digest )
   {
      producer_signature = signer.sign( signed_digest( schedule_digest ) );
   }

   bool signed_block_header::validate_signee(const public_key_type& expected_signee, const fc::sha256& schedule_digest )const
   {
      return signee( schedule_digest ) == expected_signee;
   }

} }
