/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <eos/chain/block.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <algorithm>

namespace eos { namespace chain {
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

   fc::ecc::public_key signed_block_header::signee()const
   {
      return fc::ecc::public_key(producer_signature, digest(), true/*enforce canonical*/);
   }

   void signed_block_header::sign(const fc::ecc::private_key& signer)
   {
      producer_signature = signer.sign_compact(digest());
   }

   bool signed_block_header::validate_signee(const fc::ecc::public_key& expected_signee)const
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

   checksum_type signed_block::calculate_merkle_root()const
   {
      if(cycles.empty())
         return checksum_type();

      vector<digest_type> ids;
      for (const auto& cycle : cycles)
         for (const auto& thread : cycle)
            ids.emplace_back(thread.merkle_digest());

/**
 *  Suggest moving thread::merkle_digest code to return vector of ids which get added to ids above and then calculating root over all
 */
#warning TODO  The merkle root needs to be over all transactions, but this is currently hashing all threads merkle roots which will make proofs O(N) rather than O( LOG(N) )

      /**
       *  This may require passing 
       */
#warning TODO  Add global incremental block header merkle   https://github.com/EOSIO/eos/issues/8

      return checksum_type::hash(merkle(ids));
   }

   digest_type thread::merkle_digest() const {
      vector<digest_type> ids;// = generated_input;
      /*
      ids.reserve( ids.size() + user_input.size() + output_transactions.size() );
      */

      for( const auto& trx : user_input )
         ids.push_back( transaction_digest(trx) );

      for( const auto& trx : generated_input )
         ids.push_back( trx.id );


      return merkle(ids);
   }

} }
