#pragma once
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/restrict.hpp>
#include <fc/io/raw.hpp>
#include <eosio/state_history/binary_stream.hpp>
#include <eosio/state_history/bio_device_adaptor.hpp>


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
      uint32_t end_pos = strm.tellp();
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
      fc::binary_stream<bio::filtering_ostreambuf> compressed_strm(bio::zlib_compressor() | fc::to_sink(strm));
      fc::raw::pack(compressed_strm, obj);
   }
}

template <typename STREAM, typename T>
void zlib_unpack(STREAM& strm, T& obj) {
   uint32_t len;
   fc::raw::unpack(strm, len);
   if (len > 0) {
      fc::binary_stream<bio::filtering_istreambuf> decompress_strm(bio::zlib_decompressor() | bio::restrict(fc::to_source(strm), 0, len));
      fc::raw::unpack(decompress_strm, obj);
   }
}

} // namespace state_history
} // namespace eosio
