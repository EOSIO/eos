/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

#include <boost/range/adaptor/transformed.hpp>

namespace eosio { namespace chain {

void transaction_header::set_reference_block( const block_id_type& reference_block ) {
   ref_block_num    = fc::endian_reverse_u32(reference_block._hash[0]);
   ref_block_prefix = reference_block._hash[1];
}

bool transaction_header::verify_reference_block( const block_id_type& reference_block )const {
   return ref_block_num    == (decltype(ref_block_num))fc::endian_reverse_u32(reference_block._hash[0]) &&
          ref_block_prefix == (decltype(ref_block_prefix))reference_block._hash[1];
}


transaction_id_type transaction::id() const { 
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}


digest_type transaction::sig_digest( const chain_id_type& chain_id )const {
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return enc.result();
}

const signature_type& signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign(sig_digest(chain_id)));
   return signatures.back();
}

signature_type signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign(sig_digest(chain_id));
}

flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   using boost::adaptors::transformed;
   auto sig_to_key = transformed([digest = sig_digest(chain_id)](const fc::crypto::signature& signature) {
      return public_key_type(signature, digest);
   });
   auto key_range = signatures | sig_to_key;
   return {key_range.begin(), key_range.end()};
} FC_CAPTURE_AND_RETHROW() }


} } // eosio::chain
