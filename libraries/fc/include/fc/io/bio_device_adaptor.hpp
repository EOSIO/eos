#pragma once

#include <boost/iostreams/categories.hpp>
namespace fc {

namespace bio = boost::iostreams;

template <typename STREAM, typename Category>
struct device_adaptor {
   STREAM& strm;

   typedef char     char_type;
   typedef Category category;
   size_t           write(const char* data, size_t n) { return strm.write(data, n), n; }
   size_t           read(char* data, size_t n) { return strm.read(data, n), n; }
};

template <typename STREAM>
device_adaptor<STREAM, bio::sink_tag> to_sink(STREAM& strm) {
   return device_adaptor<STREAM, bio::sink_tag>{strm};
}

template <typename STREAM>
device_adaptor<STREAM, bio::source_tag> to_source(STREAM& strm) {
   return device_adaptor<STREAM, bio::source_tag>{strm};
}

template <typename STREAM>
device_adaptor<STREAM, bio::source_tag> to_seekable(STREAM& strm) {
   return device_adaptor<STREAM, bio::seekable_device_tag>{strm};
}
} // namespace fc