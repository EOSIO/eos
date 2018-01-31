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
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>


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

flat_set<public_key_type> transaction::get_signature_keys( const vector<signature_type>& signatures, const chain_id_type& chain_id )const
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


const signature_type& signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign(sig_digest(chain_id)));
   return signatures.back();
}

signature_type signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign(sig_digest(chain_id));
}

flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{
   return transaction::get_signature_keys(signatures, chain_id);
}

namespace bio = boost::iostreams;

template<size_t Limit>
struct read_limiter {
   using char_type = char;
   using category = bio::multichar_output_filter_tag;

   template<typename Sink>
   size_t write(Sink &sink, const char* s, size_t count)
   {
      EOS_ASSERT(_total + count <= Limit, tx_decompression_error, "Exceeded maximum decompressed transaction size");
      _total += count;
      return bio::write(sink, s, count);
   }

   size_t _total = 0;
};

static transaction unpack_transaction(const bytes& data) {
   return fc::raw::unpack<transaction>(data);
}


static bytes zlib_decompress(const bytes& data) {
   try {
      bytes out;
      bio::filtering_ostream decomp;
      decomp.push(bio::zlib_decompressor());
      decomp.push(read_limiter<1*1024*1024>()); // limit to 10 megs decompressed for zip bomb protections
      decomp.push(bio::back_inserter(out));
      bio::write(decomp, data.data(), data.size());
      bio::close(decomp);
      return out;
   } catch( fc::exception& er ) {
      throw;
   } catch( ... ) {
      fc::unhandled_exception er( FC_LOG_MESSAGE( warn, "internal decompression error"), std::current_exception() );
      throw er;
   }
}

static transaction zlib_decompress_transaction(const bytes& data) {
   bytes out = zlib_decompress(data);
   return unpack_transaction(out);
}

static bytes pack_transaction(const transaction& t) {
   return fc::raw::pack(t);
}

static bytes zlib_compress_transaction(const transaction& t) {
   bytes in = pack_transaction(t);
   bytes out;
   bio::filtering_ostream comp;
   comp.push(bio::zlib_compressor(bio::zlib::best_compression));
   comp.push(bio::back_inserter(out));
   bio::write(comp, in.data(), in.size());
   bio::close(comp);
   return out;
}

bytes packed_transaction::get_raw_transaction() const
{
   try {
      switch(compression) {
         case none:
            return data;
         case zlib:
            return zlib_decompress(data);
         default:
            FC_THROW("Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(data))
}

transaction packed_transaction::get_transaction()const
{
   try {
      switch(compression) {
         case none:
            return unpack_transaction(data);
         case zlib:
            return zlib_decompress_transaction(data);
         default:
            FC_THROW("Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(data))
}

signed_transaction packed_transaction::get_signed_transaction() const
{
   return signed_transaction(get_transaction(), signatures);
}

void packed_transaction::set_transaction(const transaction& t, packed_transaction::compression_type _compression)
{
   try {
      switch(_compression) {
         case none:
            data = pack_transaction(t);
            break;
         case zlib:
            data = zlib_compress_transaction(t);
            break;
         default:
            FC_THROW("Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((_compression)(t))
   compression = _compression;
}


} } // eosio::chain
