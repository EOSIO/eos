#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <algorithm>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {

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
   auto start = fc::now();
   recovered_pub_keys.clear();
   const digest_type digest = sig_digest(chain_id, cfd);

   for(const signature_type& sig : signatures) {
      auto now = fc::now();
      EOS_ASSERT( now < deadline, tx_cpu_usage_exceeded, "transaction signature verification executed for too long ${time}us",
                  ("time", now - start)("now", now)("deadline", deadline)("start", start) );
      auto[ itr, successful_insertion ] = recovered_pub_keys.emplace( sig, digest );
      EOS_ASSERT( allow_duplicate_keys || successful_insertion, tx_duplicate_sig,
                  "transaction includes more than one signature signed using the same key associated with public key: ${key}",
                  ("key", *itr ) );
   }

   return fc::now() - start;
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

uint32_t packed_transaction_v0::get_unprunable_size()const {
   uint64_t size = config::fixed_net_overhead_of_packed_trx;
   size += packed_trx.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

uint32_t packed_transaction_v0::get_prunable_size()const {
   uint64_t size = fc::raw::pack_size(signatures);
   size += packed_context_free_data.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

digest_type packed_transaction_v0::packed_digest()const {
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

packed_transaction_v0::packed_transaction_v0(const bytes& packed_txn, const vector<signature_type>& sigs, const bytes& packed_cfd, compression_type _compression)
:signatures(sigs)
,compression(_compression)
,packed_context_free_data(packed_cfd)
,packed_trx(packed_txn)
{
   local_unpack_transaction({});
   if( !packed_context_free_data.empty() ) {
      local_unpack_context_free_data();
   }
}

packed_transaction_v0::packed_transaction_v0( bytes&& packed_txn, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression )
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

packed_transaction_v0::packed_transaction_v0( bytes&& packed_txn, vector<signature_type>&& sigs, vector<bytes>&& cfd, compression_type _compression )
:signatures(std::move(sigs))
,compression(_compression)
,packed_trx(std::move(packed_txn))
{
   local_unpack_transaction( std::move( cfd ) );
   if( !unpacked_trx.context_free_data.empty() ) {
      local_pack_context_free_data();
   }
}

packed_transaction_v0::packed_transaction_v0( transaction&& t, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression )
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

void packed_transaction_v0::reflector_init()
{
   // called after construction, but always on the same thread and before packed_transaction passed to any other threads
   static_assert(fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                 "FC unpack needs to call reflector_init otherwise unpacked_trx will not be initialized");
   EOS_ASSERT( unpacked_trx.expiration == time_point_sec(), tx_decompression_error, "packed_transaction_v0 already unpacked" );
   local_unpack_transaction({});
   local_unpack_context_free_data();
}

static transaction unpack_transaction(const bytes& packed_trx, packed_transaction_v0::compression_type compression) {
   try {
      switch( compression ) {
         case packed_transaction_v0::compression_type::none:
            return unpack_transaction( packed_trx );
         case packed_transaction_v0::compression_type::zlib:
            return zlib_decompress_transaction( packed_trx );
         default:
            EOS_THROW( unknown_transaction_compression, "Unknown transaction compression algorithm" );
      }
   } FC_CAPTURE_AND_RETHROW( (compression) )
}

void packed_transaction_v0::local_unpack_transaction(vector<bytes>&& context_free_data)
{
   unpacked_trx = signed_transaction( unpack_transaction( packed_trx, compression ), signatures, std::move(context_free_data) );
   trx_id = unpacked_trx.id();
}

static vector<bytes> unpack_context_free_data(const bytes& packed_context_free_data, packed_transaction_v0::compression_type compression)
{
   try {
      switch( compression ) {
         case packed_transaction_v0::compression_type::none:
            return unpack_context_free_data( packed_context_free_data );
         case packed_transaction_v0::compression_type::zlib:
            return zlib_decompress_context_free_data( packed_context_free_data );
         default:
            EOS_THROW( unknown_transaction_compression, "Unknown transaction compression algorithm" );
      }
   } FC_CAPTURE_AND_RETHROW( (compression) )
}

void packed_transaction_v0::local_unpack_context_free_data()
{
   EOS_ASSERT(unpacked_trx.context_free_data.empty(), tx_decompression_error, "packed_transaction.context_free_data not empty");
   unpacked_trx.context_free_data = unpack_context_free_data( packed_context_free_data, compression );
}

static bytes pack_transaction(const transaction& trx, packed_transaction_v0::compression_type compression) {
   try {
      switch(compression) {
         case packed_transaction_v0::compression_type::none: {
            return pack_transaction( trx );
         }
         case packed_transaction_v0::compression_type::zlib:
            return zlib_compress_transaction(trx);
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression))
}

void packed_transaction_v0::local_pack_transaction()
{
   packed_trx = pack_transaction(unpacked_trx, compression);
}

static bytes pack_context_free_data( const vector<bytes>& cfd, packed_transaction_v0::compression_type compression ) {
   try {
      switch(compression) {
         case packed_transaction_v0::compression_type::none:
            return pack_context_free_data(cfd);
         case packed_transaction_v0::compression_type::zlib:
            return zlib_compress_context_free_data(cfd);
         default:
            EOS_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
      }
   } FC_CAPTURE_AND_RETHROW((compression))
}

void packed_transaction_v0::local_pack_context_free_data()
{
   packed_context_free_data = pack_context_free_data(unpacked_trx.context_free_data, compression);
}


static digest_type prunable_digest(const packed_transaction::prunable_data_type::none& obj) {
   return obj.prunable_digest;
}

static digest_type prunable_digest(const packed_transaction::prunable_data_type::partial& obj) {
   EOS_THROW(tx_prune_exception, "unimplemented");
}

static digest_type prunable_digest(const packed_transaction::prunable_data_type::full& obj) {
   EOS_THROW(tx_prune_exception, "unimplemented");
}

static digest_type prunable_digest(const packed_transaction::prunable_data_type::full_legacy& obj) {
   digest_type::encoder prunable;
   fc::raw::pack( prunable, obj.signatures );
   fc::raw::pack( prunable, obj.packed_context_free_data );
   return prunable.result();
}

digest_type packed_transaction::prunable_data_type::digest() const {
   return prunable_data.visit( [](const auto& obj) { return prunable_digest(obj); } );
}

packed_transaction::prunable_data_type packed_transaction::prunable_data_type::prune_all() const {
   return prunable_data.visit(
       [](const auto& obj) -> packed_transaction::prunable_data_type { return {none{prunable_digest(obj)}}; });
}

static constexpr std::size_t digest_pack_size = 32;

static std::size_t padded_pack_size(const packed_transaction::prunable_data_type::none& obj, packed_transaction::prunable_data_type::compression_type) {
   std::size_t result = fc::raw::pack_size(obj);
   EOS_ASSERT(result == digest_pack_size, packed_transaction_type_exception, "Unexpected size of packed digest");
   return result;
}


static std::size_t padded_pack_size(const packed_transaction::prunable_data_type::partial& obj, packed_transaction::prunable_data_type::compression_type segment_compression) {
     EOS_THROW(tx_prune_exception, "unimplemented");
}

static std::size_t padded_pack_size(const packed_transaction::prunable_data_type::full& obj, packed_transaction::prunable_data_type::compression_type segment_compression) {
   EOS_THROW(tx_prune_exception, "unimplemented");
#if 0
   std::size_t context_free_size = fc::raw::pack_size(fc::unsigned_int(obj.context_free_segments.size()));
   context_free_size += obj.context_free_segments.size();
   for(const std::vector<char>& v: obj.context_free_segments) {
      // TODO: Handle segment_compression
      context_free_size += std::max(fc::raw::pack_size(v), digest_pack_size);
   }
   return fc::raw::pack_size(obj.signatures) + std::max(context_free_size, digest_pack_size);
#endif
}

static std::size_t padded_pack_size(const packed_transaction::prunable_data_type::full_legacy& obj, packed_transaction::prunable_data_type::compression_type) {
  return std::max(fc::raw::pack_size(obj), digest_pack_size);
}

std::size_t packed_transaction::prunable_data_type::maximum_pruned_pack_size(packed_transaction::prunable_data_type::compression_type compression) const {
   return 1 + prunable_data.visit([&](const auto& t){ return padded_pack_size(t, compression); });
}

static packed_transaction::prunable_data_type make_prunable_transaction_data( bool legacy, vector<signature_type> signatures,
                                                                              vector<bytes> context_free_data,
                                                                              packed_transaction::compression_type _compression ) {
   if(legacy) {
      bytes packed_cfd = pack_context_free_data( context_free_data, _compression );
      return {packed_transaction::prunable_data_type::full_legacy{std::move( signatures), std::move( packed_cfd), std::move( context_free_data) } };
   } else {
      return {packed_transaction::prunable_data_type::full{std::move( signatures), std::move( context_free_data) } };
   }
}

packed_transaction::packed_transaction(signed_transaction t, bool legacy, compression_type _compression)
 : compression(_compression),
   prunable_data(make_prunable_transaction_data(legacy, std::move(t.signatures), std::move(t.context_free_data), _compression)),
   packed_trx(pack_transaction(t, compression)),
   unpacked_trx(std::move(t)),
   trx_id(unpacked_trx.id())
{
   estimated_size = calculate_estimated_size();
}

packed_transaction::packed_transaction(const packed_transaction_v0& other, bool legacy)
 : compression(other.compression),
   prunable_data(legacy ? prunable_data_type{prunable_data_type::full_legacy{other.signatures,
                                                                             other.packed_context_free_data,
                                                                             other.unpacked_trx.context_free_data } }
                        : prunable_data_type{prunable_data_type::full{other.signatures,
                                                                      other.unpacked_trx.context_free_data } }),
   packed_trx(other.packed_trx),
   unpacked_trx(other.unpacked_trx),
   trx_id(other.id())
{
   estimated_size = calculate_estimated_size();
}

packed_transaction::packed_transaction(packed_transaction_v0&& other, bool legacy)
 : compression(other.compression),
   prunable_data(legacy ? prunable_data_type{prunable_data_type::full_legacy{std::move(other.signatures),
                                                                             std::move(other.packed_context_free_data),
                                                                             std::move(other.unpacked_trx.context_free_data) } }
                        : prunable_data_type{prunable_data_type::full{std::move(other.signatures),
                                                                      std::move(other.unpacked_trx.context_free_data) } }),
   packed_trx(std::move(other.packed_trx)),
   unpacked_trx(std::move(other.unpacked_trx)),
   trx_id(other.id())
{
   estimated_size = calculate_estimated_size();
}

packed_transaction_v0_ptr packed_transaction::to_packed_transaction_v0() const {
   const auto* sigs = get_signatures();
   const auto* context_free_data = get_context_free_data();
   signed_transaction strx( transaction( get_transaction() ),
                            sigs != nullptr ? *sigs : vector<signature_type>(),
                            context_free_data != nullptr ? *context_free_data : vector<bytes>() );
   return std::make_shared<const packed_transaction_v0>( std::move( strx ), get_compression() );
}

uint32_t packed_transaction::get_unprunable_size()const {
   uint64_t size = config::fixed_net_overhead_of_packed_trx;
   size += packed_trx.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

static uint32_t get_prunable_size_impl(const packed_transaction::prunable_data_type::full_legacy& obj) {
   uint64_t size = fc::raw::pack_size(obj.signatures);
   size += obj.packed_context_free_data.size();
   EOS_ASSERT( size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big" );
   return static_cast<uint32_t>(size);
}

static uint32_t get_prunable_size_impl(const packed_transaction::prunable_data_type::full&) {
   EOS_THROW(tx_prune_exception, "unimplemented");
}

template<typename T>
static uint32_t get_prunable_size_impl(const T&) {
   EOS_THROW(tx_prune_exception, "unknown size: prunable data has been pruned.");
}

uint32_t packed_transaction::get_prunable_size()const {
  return prunable_data.prunable_data.visit([](const auto& obj) { return get_prunable_size_impl(obj); });
}

uint32_t packed_transaction::calculate_estimated_size() const {
   auto est_size = overloaded{
      [](const std::vector<bytes>& vec) {
         uint32_t s = 0;
         for( const auto& v : vec ) s += v.size();
         return s;
      },
      [](const std::vector<prunable_data_type::segment_type>& vec) {
         uint32_t s = 0;
         for( const auto& v : vec ) {
            s += v.visit( overloaded{
               [](const digest_type& t) { return sizeof(t); },
               [](const bytes& vec) { return vec.size(); }
            } );
         }
         return s;
      }
   };
   auto visitor = overloaded{
         [](const prunable_data_type::none& v) { return 0ul; },
         [&](const prunable_data_type::partial& v) {
            return v.signatures.size() * sizeof(signature_type) + est_size(v.context_free_segments);
         },
         [&](const prunable_data_type::full& v) {
            return v.signatures.size() * sizeof(signature_type) + est_size(v.context_free_segments);
         },
         [&](const prunable_data_type::full_legacy& v) {
            return v.signatures.size() * sizeof(signature_type) + v.packed_context_free_data.size() + est_size(v.context_free_segments);
         }
   };

   return sizeof(*this) + packed_trx.size() * 2 + prunable_data.prunable_data.visit(visitor);
}

digest_type packed_transaction::packed_digest()const {
   digest_type::encoder enc;
   fc::raw::pack( enc, compression );
   fc::raw::pack( enc, packed_trx  );
   fc::raw::pack( enc, prunable_data.digest() );
   return enc.result();
}

template<typename T>
static auto maybe_get_signatures(const T& obj) -> decltype(&obj.signatures) { return &obj.signatures; }
static auto maybe_get_signatures(const packed_transaction::prunable_data_type::none&) -> const std::vector<signature_type>* { return nullptr; }
const vector<signature_type>* packed_transaction::get_signatures()const {
   return prunable_data.prunable_data.visit([](const auto& obj) { return maybe_get_signatures(obj); });
}

const vector<bytes>* packed_transaction::get_context_free_data()const {
   if( prunable_data.prunable_data.contains<prunable_data_type::full>() ) {
      return &prunable_data.prunable_data.get<prunable_data_type::full>().context_free_segments;
   } else if( prunable_data.prunable_data.contains<prunable_data_type::full_legacy>() ) {
      return &prunable_data.prunable_data.get<prunable_data_type::full_legacy>().context_free_segments;
   } else {
      return nullptr;
   }
}

const bytes* maybe_get_context_free_data(const packed_transaction::prunable_data_type::none&, std::size_t) { return nullptr; }
const bytes* maybe_get_context_free_data(const packed_transaction::prunable_data_type::partial& p, std::size_t i) {
   if( p.context_free_segments.size() <= i ) return nullptr;
   return p.context_free_segments[i].visit(
         overloaded{
               []( const digest_type& t ) -> const bytes* { return nullptr; },
               []( const bytes& vec ) { return &vec; }
         } );
}
const bytes* maybe_get_context_free_data(const packed_transaction::prunable_data_type::full_legacy& full_leg, std::size_t i) {
   if( full_leg.context_free_segments.size() <= i ) return nullptr;
   return &full_leg.context_free_segments[i];
}
const bytes* maybe_get_context_free_data(const packed_transaction::prunable_data_type::full& f, std::size_t i) {
   if( f.context_free_segments.size() <= i ) return nullptr;
   return &f.context_free_segments[i];
}

const bytes* packed_transaction::get_context_free_data(std::size_t segment_ordinal) const {
  return prunable_data.prunable_data.visit([&](const auto& obj) { return maybe_get_context_free_data(obj, segment_ordinal); });
}

void packed_transaction::prune_all() {
   prunable_data = prunable_data.prune_all();
}

std::size_t packed_transaction::maximum_pruned_pack_size(cf_compression_type segment_compression) const {
   return fc::raw::pack_size(compression) + fc::raw::pack_size(packed_trx) + prunable_data.maximum_pruned_pack_size(segment_compression);
}

void packed_transaction::reflector_init()
{
   // called after construction, but always on the same thread and before packed_transaction passed to any other threads
   static_assert(fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                 "FC unpack needs to call reflector_init otherwise unpacked_trx will not be initialized");
   EOS_ASSERT( unpacked_trx.expiration == time_point_sec(), tx_decompression_error, "packed_transaction already unpacked" );
   unpacked_trx = unpack_transaction(packed_trx, compression);
   trx_id = unpacked_trx.id();
   if( prunable_data.prunable_data.contains<prunable_data_type::full_legacy>() ) {
      auto& legacy = prunable_data.prunable_data.get<prunable_data_type::full_legacy>();
      legacy.context_free_segments = unpack_context_free_data( legacy.packed_context_free_data, compression );
   }
   estimated_size = calculate_estimated_size();
}
} } // eosio::chain
