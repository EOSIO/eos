#pragma once
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/bio_device_adaptor.hpp>


namespace eosio {
namespace state_history {

namespace bio = boost::iostreams;
template <typename STREAM>
struct length_writer {
   STREAM&  strm;
   uint64_t start_pos = 0;

   length_writer(STREAM& f)
       : strm(f) {
      uint32_t len = 0;
      strm.write((const char*)&len, sizeof(len));
      start_pos = strm.tellp();
   }

   ~length_writer() {
      uint64_t end_pos = strm.tellp();
      uint32_t len     = end_pos - start_pos;
      strm.seekp(start_pos - sizeof(len));
      strm.write((char*)&len, sizeof(len));
      strm.seekp(end_pos);
   }
};

template <typename T>
bool is_empty(const T&) {
   return false;
}
template <typename T>
bool is_empty(const std::vector<T>& obj) {
   return obj.empty();
}

template <typename STREAM, typename T>
void zlib_pack(STREAM& strm, const T& obj) {
   if (is_empty(obj)) {
      fc::raw::pack(strm, uint32_t(0));
   }
   else {
      length_writer<STREAM>     len_writer(strm);
      fc::datastream<bio::filtering_ostreambuf> compressed_strm(bio::zlib_compressor() | fc::to_sink(strm));
      fc::raw::pack(compressed_strm, obj);
   }
}

template <typename STREAM, typename T>
void zlib_unpack(STREAM& strm, T& obj) {
   uint32_t len;
   fc::raw::unpack(strm, len);
   auto pos = strm.tellp();
   if (len > 0) {
      fc::datastream<bio::filtering_istreambuf> decompress_strm(bio::zlib_decompressor() | bio::restrict(fc::to_source(strm), 0, len));
      fc::raw::unpack(decompress_strm, obj);
      // zlib may add some padding at the end of the uncompressed data so that the position of `strm` wont be at the start of next entry,
      // we need to use `seek()` to adjust the position of `strm`.
      strm.seekp(pos + len);
   }
}

template <typename STREAM>
std::vector<char> zlib_decompress(STREAM& strm) {
   uint32_t          len;
   fc::raw::unpack(strm, len);
   if (len > 0) {
      std::vector<char>         result;
      bio::filtering_istreambuf decompress_buf(bio::zlib_decompressor() | bio::restrict(fc::to_source(strm), 0, len));
      bio::copy(decompress_buf, bio::back_inserter(result));
      return result;
   }
   return {};
}

} // namespace state_history
} // namespace eosio
