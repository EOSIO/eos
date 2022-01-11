#include <eosio/state_history/compression.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace eosio {
namespace state_history {

namespace bio = boost::iostreams;
bytes zlib_compress_bytes(const bytes& in) {
   bytes                  out;
   bio::filtering_ostream comp;
   comp.push(bio::zlib_compressor(bio::zlib::default_compression));
   comp.push(bio::back_inserter(out));
   bio::write(comp, in.data(), in.size());
   bio::close(comp);
   return out;
}

bytes zlib_decompress(const bytes& in) {
   bytes                  out;
   bio::filtering_ostream decomp;
   decomp.push(bio::zlib_decompressor());
   decomp.push(bio::back_inserter(out));
   bio::write(decomp, in.data(), in.size());
   bio::close(decomp);
   return out;
}

} // namespace state_history
} // namespace eosio
