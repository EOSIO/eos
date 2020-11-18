#pragma once

#include <fstream>
#include <iostream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

namespace eosio {
namespace blockvault {

struct zlib_compressor {

   template <typename Filter>
   std::string convert(const char* input_filename, const char* output_file_suffix) {
      using namespace std;

      std::ifstream                                infile(input_filename, ios_base::in | ios_base::binary);
      boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
      in.push(Filter());
      in.push(infile);

      std::string out_filename = std::string(input_filename) + output_file_suffix;
      ofstream    outfile(out_filename, ios_base::out | ios_base::binary);
      boost::iostreams::copy(in, outfile);
      return out_filename;
   }

   std::string compress(const char* snapshot_filename) {
      return convert<boost::iostreams::zlib_compressor>(snapshot_filename, ".z");
   }

   std::string decompress(const char* compressed_file) {
      return convert<boost::iostreams::zlib_decompressor>(compressed_file, ".bin");
   }
};

} // namespace blockvault
} // namespace eosio
