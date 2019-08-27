#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>
#include <mutex>

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
   fc::microseconds cpu_usage;
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

void deferred_transaction_generation_context::reflector_init() {
      static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                     "deferred_transaction_generation_context expects FC to support reflector_init" );


      EOS_ASSERT( sender != account_name(), ill_formed_deferred_transaction_generation_context,
                  "Deferred transaction generation context extension must have a non-empty sender account",
      );
}

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

fc::microseconds transaction::get_signature_keys( const vector<signature_type>& signatures,
      const chain_id_type& chain_id, fc::time_point deadline, const vector<bytes>& cfd,
      flat_set<public_key_type>& recovered_pub_keys, bool allow_duplicate_keys)const
{ try {
   using boost::adaptors::transformed;

   constexpr size_t recovery_cache_size = 10000;
   static recovery_cache_type recovery_cache;
   static std::mutex cache_mtx;

   auto start = fc::time_point::now();
   recovered_pub_keys.clear();
   const digest_type digest = sig_digest(chain_id, cfd);

   std::unique_lock<std::mutex> lock(cache_mtx, std::defer_lock);
   fc::microseconds sig_cpu_usage;
   for(const signature_type& sig : signatures) {
      auto now = fc::time_point::now();
      EOS_ASSERT( now < deadline, tx_cpu_usage_exceeded, "transaction signature verification executed for too long ${time}us",
                  ("time", now - start)("now", now)("deadline", deadline)("start", start) );
      public_key_type recov;
      const auto& tid = id();
      lock.lock();
      recovery_cache_type::index<by_sig>::type::iterator it = recovery_cache.get<by_sig>().find( sig );
      if( it == recovery_cache.get<by_sig>().end() || it->trx_id != tid ) {
         lock.unlock();
         recov = public_key_type( sig, digest );
         fc::microseconds cpu_usage = fc::time_point::now() - start;
         lock.lock();
         recovery_cache.emplace_back( cached_pub_key{tid, recov, sig, cpu_usage} ); //could fail on dup signatures; not a problem
         sig_cpu_usage += cpu_usage;
      } else {
         recov = it->pub_key;
         sig_cpu_usage += it->cpu_usage;
      }
      lock.unlock();
      bool successful_insertion = false;
      std::tie(std::ignore, successful_insertion) = recovered_pub_keys.insert(recov);
      EOS_ASSERT( allow_duplicate_keys || successful_insertion, tx_duplicate_sig,
                  "transaction includes more than one signature signed using the same key associated with public key: ${key}",
                  ("key", recov) );
   }

   lock.lock();
   while ( recovery_cache.size() > recovery_cache_size )
      recovery_cache.erase( recovery_cache.begin());
   lock.unlock();

   return sig_cpu_usage;
} FC_CAPTURE_AND_RETHROW() }

flat_multimap<uint16_t, transaction_extension> transaction::validate_and_extract_extensions()const {
   using decompose_t = transaction_extension_types::decompose_t;

   flat_multimap<uint16_t, transaction_extension> results;

   uint16_t id_type_lower_bound = 0;

   for( size_t i = 0; i < transaction_extensions.size(); ++i ) {
      const auto& e = transaction_extensions[i];
      auto id = e.first;

      EOS_ASSERT( id >= id_type_lower_bound, invalid_transaction_extension,
                  "Transaction extensions are not in the correct order (ascending id types required)"
      );

      auto iter = results.emplace(std::piecewise_construct,
         std::forward_as_tuple(id),
         std::forward_as_tuple()
      );

      auto match = decompose_t::extract<transaction_extension>( id, e.second, iter->second );
      EOS_ASSERT( match, invalid_transaction_extension,
                  "Transaction extension with id type ${id} is not supported",
                  ("id", id)
      );

      if( match->enforce_unique ) {
         EOS_ASSERT( i == 0 || id > id_type_lower_bound, invalid_transaction_extension,
                     "Transaction extension with id type ${id} is not allowed to repeat",
                     ("id", id)
         );
      }

      id_type_lower_bound = id;
   }

   return results;
}

const signature_type& signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
   signatures.push_back(key.sign(sig_digest(chain_id, context_free_data)));
   return signatures.back();
}

signature_type signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const {
   return key.sign(sig_digest(chain_id, context_free_data));
}

fc::microseconds
signed_transaction::get_signature_keys( const chain_id_type& chain_id, fc::time_point deadline,
                                        flat_set<public_key_type>& recovered_pub_keys,
                                        bool allow_duplicate_keys)const
{
   return transaction::get_signature_keys(signatures, chain_id, deadline, context_free_data, recovered_pub_keys, allow_duplicate_keys);
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
      decomp.push(read_limiter<1*1024*1024>()); // limit to 1 meg decompressed for zip bomb protections
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
         case compression_type::none:
            return packed_trx;
         case compression_type::zlib:
            return zlib_decompress(packed_trx);
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression)(packed_trx))
}

packed_transaction::packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression )
:signatures(std::move(sigs))
,compression(_compression)
,packed_context_free_data(std::move(packed_cfd))
,packed_trx(std::move(packed_txn))
{
   local_unpack_transaction({});
   if( !packed_context_free_data.empty() ) {
      local_unpack_context_free_data();
   }
}

packed_transaction::packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, vector<bytes>&& cfd, compression_type _compression )
:signatures(std::move(sigs))
,compression(_compression)
,packed_trx(std::move(packed_txn))
{
   local_unpack_transaction( std::move( cfd ) );
   if( !unpacked_trx.context_free_data.empty() ) {
      local_pack_context_free_data();
   }
}

packed_transaction::packed_transaction( transaction&& t, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression )
:signatures(std::move(sigs))
,compression(_compression)
,packed_context_free_data(std::move(packed_cfd))
,unpacked_trx(std::move(t), signatures, {})
,trx_id(unpacked_trx.id())
{
   local_pack_transaction();
   if( !packed_context_free_data.empty() ) {
      local_unpack_context_free_data();
   }
}

void packed_transaction::reflector_init()
{
   // called after construction, but always on the same thread and before packed_transaction passed to any other threads
   static_assert(fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                 "FC unpack needs to call reflector_init otherwise unpacked_trx will not be initialized");
   EOS_ASSERT( unpacked_trx.expiration == time_point_sec(), tx_decompression_error, "packed_transaction already unpacked" );
   local_unpack_transaction({});
   local_unpack_context_free_data();
}

void packed_transaction::local_unpack_transaction(vector<bytes>&& context_free_data)
{
   try {
      switch( compression ) {
         case compression_type::none:
            unpacked_trx = signed_transaction( unpack_transaction( packed_trx ), signatures, std::move(context_free_data) );
            break;
         case compression_type::zlib:
            unpacked_trx = signed_transaction( zlib_decompress_transaction( packed_trx ), signatures, std::move(context_free_data) );
            break;
         default:
            EOS_THROW( unknown_transaction_compression, "Unknown transaction compression algorithm" );
      }
      trx_id = unpacked_trx.id();
   } FC_CAPTURE_AND_RETHROW( (compression) )
}

void packed_transaction::local_unpack_context_free_data()
{
   try {
      EOS_ASSERT(unpacked_trx.context_free_data.empty(), tx_decompression_error, "packed_transaction.context_free_data not empty");
      switch( compression ) {
         case compression_type::none:
            unpacked_trx.context_free_data = unpack_context_free_data( packed_context_free_data );
            break;
         case compression_type::zlib:
            unpacked_trx.context_free_data = zlib_decompress_context_free_data( packed_context_free_data );
            break;
         default:
            EOS_THROW( unknown_transaction_compression, "Unknown transaction compression algorithm" );
      }
   } FC_CAPTURE_AND_RETHROW( (compression) )
}

void packed_transaction::local_pack_transaction()
{
   try {
      switch(compression) {
         case compression_type::none:
            packed_trx = pack_transaction(unpacked_trx);
            break;
         case compression_type::zlib:
            packed_trx = zlib_compress_transaction(unpacked_trx);
            break;
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression))
}

void packed_transaction::local_pack_context_free_data()
{
   try {
      switch(compression) {
         case compression_type::none:
            packed_context_free_data = pack_context_free_data(unpacked_trx.context_free_data);
            break;
         case compression_type::zlib:
            packed_context_free_data = zlib_compress_context_free_data(unpacked_trx.context_free_data);
            break;
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression))
}


} } // eosio::chain
