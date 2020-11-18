#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/blockvault_client_plugin/detail/zlib_compressor.hpp>
#include <fc/filesystem.hpp>
#include <fc/scoped_exit.hpp>

BOOST_AUTO_TEST_CASE(zlib_compressor_test) {

   std::vector<char> content(1024 * sizeof(int));
   std::generate(reinterpret_cast<int*>(content.data()), reinterpret_cast<int*>(content.data() + content.size()), rand);

   fc::temp_file file;
   std::filebuf  fbuf;
   fbuf.open(file.path().string().c_str(), std::ios::out | std::ios::binary);
   fbuf.sputn(content.data(), content.size());
   fbuf.close();

   eosio::blockvault::zlib_compressor compressor;
   std::string                        compressed_file = compressor.compress(file.path().string().c_str());
   auto x = fc::make_scoped_exit([&compressed_file]() { boost::filesystem::remove(compressed_file); });

   std::string decompressed_file = compressor.decompress(compressed_file.c_str());
   auto        y = fc::make_scoped_exit([&decompressed_file]() { boost::filesystem::remove(decompressed_file); });

   std::ifstream strm(decompressed_file.c_str(), std::ios_base::in | std::ios_base::binary);

   BOOST_CHECK(std::equal(std::istreambuf_iterator<char>(strm), std::istreambuf_iterator<char>(), content.cbegin(),
                          content.cend()));
}
