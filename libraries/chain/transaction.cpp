/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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

#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

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

void transaction_header::validate()const {
   EOS_ASSERT( max_net_usage_words.value < UINT32_MAX / 8UL, transaction_exception,
               "declared max_net_usage_words overflows when expanded to max net usage" );
}

transaction_id_type transaction::id() const {
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::sig_digest( const chain_id_type& chain_id, const vector<bytes>& cfd )const {
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   if( cfd.size() ) {
      fc::raw::pack( enc, digest_type::hash(cfd) );
   } else {
      fc::raw::pack( enc, digest_type() );
   }
   return enc.result();
}

flat_set<public_key_type> transaction::get_signature_keys( const vector<signature_type>& signatures,
      const chain_id_type& chain_id, const vector<bytes>& cfd, bool allow_duplicate_keys, bool use_cache )const
{ try {
   using boost::adaptors::transformed;

   constexpr size_t recovery_cache_size = 1000;
   static thread_local recovery_cache_type recovery_cache;
   const digest_type digest = sig_digest(chain_id, cfd);

   flat_set<public_key_type> recovered_pub_keys;
   for(const signature_type& sig : signatures) {
      public_key_type recov;
      if( use_cache ) {
         recovery_cache_type::index<by_sig>::type::iterator it = recovery_cache.get<by_sig>().find( sig );
         if( it == recovery_cache.get<by_sig>().end() || it->trx_id != id()) {
            recov = public_key_type( sig, digest );
            recovery_cache.emplace_back(cached_pub_key{id(), recov, sig} ); //could fail on dup signatures; not a problem
         } else {
            recov = it->pub_key;
         }
      } else {
         recov = public_key_type( sig, digest );
      }
      bool successful_insertion = false;
      std::tie(std::ignore, successful_insertion) = recovered_pub_keys.insert(recov);
      EOS_ASSERT( allow_duplicate_keys || successful_insertion, tx_duplicate_sig,
                  "transaction includes more than one signature signed using the same key associated with public key: ${key}",
                  ("key", recov)
               );
   }

   if( use_cache ) {
      while ( recovery_cache.size() > recovery_cache_size )
         recovery_cache.erase( recovery_cache.begin() );
   }

   return recovered_pub_keys;
} FC_CAPTURE_AND_RETHROW() }


const signature_type& signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign(sig_digest(chain_id, context_free_data)));
   return signatures.back();
}

signature_type signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign(sig_digest(chain_id, context_free_data));
}

flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id, bool allow_duplicate_keys, bool use_cache )const
{
   return transaction::get_signature_keys(signatures, chain_id, context_free_data, allow_duplicate_keys, use_cache);
}

uint32_t packed_transaction::get_unprunable_size()const {
   uint64_t size = config::fixed_net_overhead_of_packed_trx;
   size += packed_trx.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

uint32_t packed_transaction::get_prunable_size()const {
   uint64_t size = fc::raw::pack_size(signatures);
   size += packed_context_free_data.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

digest_type packed_transaction::packed_digest()const {
   digest_type::encoder prunable;
   fc::raw::pack( prunable, signatures );
   fc::raw::pack( prunable, packed_context_free_data );

   digest_type::encoder enc;
   fc::raw::pack( enc, compression );
   fc::raw::pack( enc, packed_trx  );
   fc::raw::pack( enc, prunable.result() );

   return enc.result();
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

static vector<bytes> unpack_context_free_data(const bytes& data) {
   if( data.size() == 0 )
      return vector<bytes>();

   return fc::raw::unpack< vector<bytes> >(data);
}

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

static vector<bytes> zlib_decompress_context_free_data(const bytes& data) {
   if( data.size() == 0 )
      return vector<bytes>();

   bytes out = zlib_decompress(data);
   return unpack_context_free_data(out);
}

static transaction zlib_decompress_transaction(const bytes& data) {
   bytes out = zlib_decompress(data);
   return unpack_transaction(out);
}

static bytes pack_transaction(const transaction& t) {
   return fc::raw::pack(t);
}

static bytes pack_context_free_data(const vector<bytes>& cfd ) {
   if( cfd.size() == 0 )
      return bytes();

   return fc::raw::pack(cfd);
}

static bytes zlib_compress_context_free_data(const vector<bytes>& cfd ) {
   if( cfd.size() == 0 )
      return bytes();

   bytes in = pack_context_free_data(cfd);
   bytes out;
   bio::filtering_ostream comp;
   comp.push(bio::zlib_compressor(bio::zlib::best_compression));
   comp.push(bio::back_inserter(out));
   bio::write(comp, in.data(), in.size());
   bio::close(comp);
   return out;
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
            return packed_trx;
         case zlib:
            return zlib_decompress(packed_trx);
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(packed_trx))
}

vector<bytes> packed_transaction::get_context_free_data()const
{
   try {
      switch(compression) {
         case none:
            return unpack_context_free_data(packed_context_free_data);
         case zlib:
            return zlib_decompress_context_free_data(packed_context_free_data);
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(packed_context_free_data))
}

time_point_sec packed_transaction::expiration()const
{
   local_unpack();
   return unpacked_trx->expiration;
}

transaction_id_type packed_transaction::id()const
{
   local_unpack();
   return get_transaction().id();
}

transaction_id_type packed_transaction::get_uncached_id()const
{
   const auto raw = get_raw_transaction();
   return fc::raw::unpack<transaction>( raw ).id();
}

void packed_transaction::local_unpack()const
{
   if (!unpacked_trx) {
      try {
         switch(compression) {
         case none:
            unpacked_trx = unpack_transaction(packed_trx);
            break;
         case zlib:
            unpacked_trx = zlib_decompress_transaction(packed_trx);
            break;
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
         }
      } FC_CAPTURE_AND_RETHROW((compression)(packed_trx))
   }
}

transaction packed_transaction::get_transaction()const
{
   local_unpack();
   return transaction(*unpacked_trx);
}

signed_transaction packed_transaction::get_signed_transaction() const
{
   try {
      switch(compression) {
         case none:
            return signed_transaction(get_transaction(), signatures, unpack_context_free_data(packed_context_free_data));
         case zlib:
            return signed_transaction(get_transaction(), signatures, zlib_decompress_context_free_data(packed_context_free_data));
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(packed_trx)(packed_context_free_data))

}

void packed_transaction::set_transaction(const transaction& t, packed_transaction::compression_type _compression)
{
   try {
      switch(_compression) {
         case none:
            packed_trx = pack_transaction(t);
            break;
         case zlib:
            packed_trx = zlib_compress_transaction(t);
            break;
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((_compression)(t))
   packed_context_free_data.clear();
   compression = _compression;
}

void packed_transaction::set_transaction(const transaction& t, const vector<bytes>& cfd, packed_transaction::compression_type _compression)
{
   try {
      switch(_compression) {
         case none:
            packed_trx = pack_transaction(t);
            packed_context_free_data = pack_context_free_data(cfd);
            break;
         case zlib:
            packed_trx = zlib_compress_transaction(t);
            packed_context_free_data = zlib_compress_context_free_data(cfd);
            break;
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((_compression)(t))
   compression = _compression;
}


} } // eosio::chain
