/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

#include <boost/range/adaptor/transformed.hpp>

namespace eosio { namespace chain {

digest_type transaction_digest(const transaction& t) {
   digest_type::encoder enc;
   fc::raw::pack( enc, t );
   return enc.result();
}

void transaction_set_reference_block(transaction& t, const block_id_type& reference_block) {
   t.ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
   t.ref_block_prefix = reference_block._hash[1];
}

bool transaction_verify_reference_block(const transaction& t, const block_id_type& reference_block) {
   return t.ref_block_num == (decltype(t.ref_block_num))fc::endian_reverse_u32(reference_block._hash[0]) &&
          t.ref_block_prefix == (decltype(t.ref_block_prefix))reference_block._hash[1];
}

digest_type signed_transaction::sig_digest( const chain_id_type& chain_id )const {
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, static_cast<const types::transaction&>(*this) );
   return enc.result();
}

eosio::chain::transaction_id_type signed_transaction::id() const {
   auto h = transaction_digest(*this);
   transaction_id_type result;
   memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
   return result;
}

const signature_type& eosio::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign_compact(sig_digest(chain_id)));
   return signatures.back();
}

signature_type eosio::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign_compact(sig_digest(chain_id));
}

flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
      using boost::adaptors::transformed;
      auto SigToKey = transformed([digest = sig_digest(chain_id)](const fc::ecc::compact_signature& signature) {
         return public_key_type(fc::ecc::public_key(signature, digest));
      });
      auto keyRange = signatures | SigToKey;
      return {keyRange.begin(), keyRange.end()};
   } FC_CAPTURE_AND_RETHROW() }

eosio::chain::digest_type signed_transaction::merkle_digest() const {
   digest_type::encoder enc;
   fc::raw::pack(enc, static_cast<const types::transaction&>(*this));
   return enc.result();
}

digest_type generated_transaction::merkle_digest() const {
   digest_type::encoder enc;
   fc::raw::pack(enc, *this);
   return enc.result();
}

} } // eosio::chain
