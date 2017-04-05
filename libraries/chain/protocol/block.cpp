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
#include <eos/chain/protocol/block.hpp>
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
      auto tmp = fc::sha224::hash( *this );
      tmp._hash[0] = fc::endian_reverse_u32(block_num()); // store the block num in the ID, 160 bits is plenty for the hash
      static_assert( sizeof(tmp._hash[0]) == 4, "should be 4 bytes" );
      block_id_type result;
      memcpy(result._hash, tmp._hash, std::min(sizeof(result), sizeof(tmp)));
      return result;
   }

   fc::ecc::public_key signed_block_header::signee()const
   {
      return fc::ecc::public_key( producer_signature, digest(), true/*enforce canonical*/ );
   }

   void signed_block_header::sign( const fc::ecc::private_key& signer )
   {
      producer_signature = signer.sign_compact( digest() );
   }

   bool signed_block_header::validate_signee( const fc::ecc::public_key& expected_signee )const
   {
      return signee() == expected_signee;
   }

   checksum_type signed_block::calculate_merkle_root()const
   {
      if( transactions.size() == 0 )
         return checksum_type();

      vector<digest_type> ids;
      ids.resize( transactions.size() );
      for( uint32_t i = 0; i < transactions.size(); ++i )
         ids[i] = transactions[i].merkle_digest();

      vector<digest_type>::size_type current_number_of_hashes = ids.size();
      while( current_number_of_hashes > 1 )
      {
         // hash ID's in pairs
         uint32_t i_max = current_number_of_hashes - (current_number_of_hashes&1);
         uint32_t k = 0;

         for( uint32_t i = 0; i < i_max; i += 2 )
            ids[k++] = digest_type::hash( std::make_pair( ids[i], ids[i+1] ) );

         if( current_number_of_hashes&1 )
            ids[k++] = ids[i_max];
         current_number_of_hashes = k;
      }
      return checksum_type::hash( ids[0] );
   }

} }
