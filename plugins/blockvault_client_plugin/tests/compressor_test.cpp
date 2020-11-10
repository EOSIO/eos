#include "../zlib_compressor.hpp"
#include "../scope_exit.hpp"
#include "temporary_file.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(zlib_compressor_test) {

   std::vector<char> content(1024 * sizeof(int));
   std::generate(reinterpret_cast<int*>(content.data()), reinterpret_cast<int*>(content.data() + content.size()), rand);

   temporary_file tmp_file(content);

   eosio::blockvault::zlib_compressor compressor;
   std::string                        compressed_file   = compressor.compress(tmp_file.name.c_str());
   auto x = eosio::blockvault::make_scope_exit([&compressed_file]() { std::filesystem::remove(compressed_file); });

   std::string                        decompressed_file = compressor.decompress(compressed_file.c_str());
   auto y = eosio::blockvault::make_scope_exit([&decompressed_file]() { std::filesystem::remove(decompressed_file); });

   std::ifstream strm(decompressed_file.c_str(), std::ios_base::in | std::ios_base::binary);

   BOOST_CHECK(std::equal(std::istreambuf_iterator<char>(strm), std::istreambuf_iterator<char>(), content.cbegin(), content.cend()));
}
