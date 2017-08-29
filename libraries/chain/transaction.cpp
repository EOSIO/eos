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
#include <eos/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

#include <boost/range/adaptor/transformed.hpp>

namespace eos { namespace chain {

digest_type transaction_digest(const Transaction& t) {
   digest_type::encoder enc;
   fc::raw::pack( enc, t );
   return enc.result();
}

void transaction_set_reference_block(Transaction& t, const block_id_type& reference_block) {
   t.refBlockNum = fc::endian_reverse_u32(reference_block._hash[0]);
   t.refBlockPrefix = reference_block._hash[1];
}

bool transaction_verify_reference_block(const Transaction& t, const block_id_type& reference_block) {
   return t.refBlockNum == (decltype(t.refBlockNum))fc::endian_reverse_u32(reference_block._hash[0]) &&
          t.refBlockPrefix == (decltype(t.refBlockPrefix))reference_block._hash[1];
}

digest_type SignedTransaction::sig_digest( const chain_id_type& chain_id )const {
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, static_cast<const types::Transaction&>(*this) );
   return enc.result();
}

eos::chain::transaction_id_type SignedTransaction::id() const {
   auto h = transaction_digest(*this);
   transaction_id_type result;
   memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
   return result;
}

const signature_type& eos::chain::SignedTransaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign_compact(sig_digest(chain_id)));
   return signatures.back();
}

signature_type eos::chain::SignedTransaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign_compact(sig_digest(chain_id));
}

flat_set<public_key_type> SignedTransaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   using boost::adaptors::transformed;
   auto SigToKey = transformed([digest = sig_digest(chain_id)](const fc::ecc::compact_signature& signature) {
      return public_key_type(fc::ecc::public_key(signature, digest));
   });
   auto keyRange = signatures | SigToKey;
   return {keyRange.begin(), keyRange.end()};
   } FC_CAPTURE_AND_RETHROW() }

eos::chain::digest_type SignedTransaction::merkle_digest() const {
   digest_type::encoder enc;
   fc::raw::pack(enc, static_cast<const types::Transaction&>(*this));
   return enc.result();
}

digest_type GeneratedTransaction::merkle_digest() const {
   digest_type::encoder enc;
   fc::raw::pack(enc, *this);
   return enc.result();
}

} } // eos::chain
