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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

namespace eosio { namespace chain {

using namespace boost::multi_index;

struct cached_pub_key {
   transaction_id_type trx_id;
   public_key_type pub_key;
   signature_type sig;
   cached_pub_key(const cached_pub_key&) = delete;
   cached_pub_key() = delete;
   cached_pub_key& operator=(const cached_pub_key&) = delete;
   cached_pub_key(cached_pub_key&&) = default;
};
struct by_sig{};

typedef multi_index_container<
   cached_pub_key,
   indexed_by<
      sequenced<>,
      hashed_unique<
         tag<by_sig>,
         member<cached_pub_key,
                signature_type,
                &cached_pub_key::sig>
      >
   >
> recovery_cache_type;

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

   constexpr size_t recovery_cache_size = 100000;
   static recovery_cache_type recovery_cache;
   const digest_type digest = sig_digest(chain_id);

   flat_set<public_key_type> recovered_pub_keys;
   for(const signature_type& sig : signatures) {
      recovery_cache_type::index<by_sig>::type::iterator it = recovery_cache.get<by_sig>().find(sig);

      if(it == recovery_cache.get<by_sig>().end() || it->trx_id != id()) {
         public_key_type recov = public_key_type(sig, digest);
         recovery_cache.emplace_back( cached_pub_key{id(), recov, sig} ); //could fail on dup signatures; not a problem
         recovered_pub_keys.insert(recov);
         continue;
      }
      recovered_pub_keys.insert(it->pub_key);
   }

   while(recovery_cache.size() > recovery_cache_size)
      recovery_cache.erase(recovery_cache.begin());

   return recovered_pub_keys;
} FC_CAPTURE_AND_RETHROW() }


} } // eosio::chain
