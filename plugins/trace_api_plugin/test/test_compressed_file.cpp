#define BOOST_TEST_MODULE trace_compressed_file
#include <boost/test/included/unit_test.hpp>
#include <list>
#include <boost/filesystem.hpp>

#include <eosio/trace_api/compressed_file.hpp>
#include <eosio/trace_api/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api;

namespace bfs = boost::filesystem;

struct temp_file_fixture {
   temp_file_fixture() {}

   ~temp_file_fixture() {
      for (const auto& p: paths) {
         if (bfs::exists(p)) {
            bfs::remove(p);
         }
      }
   }

   std::string create_temp_file( const std::string& contents ) {
      auto path = bfs::temp_directory_path() / bfs::unique_path();
      auto os = bfs::ofstream(path, std::ios_base::out);
      os << contents;
      os.close();
      return paths.emplace_back(std::move(path)).generic_string();
   }

   std::string create_temp_file( const void* data, size_t size ) {
      auto path = bfs::temp_directory_path() / bfs::unique_path();
      auto os = bfs::ofstream(path, std::ios_base::out|std::ios_base::binary);
      if (data && size)
         os.write(reinterpret_cast<const char*>(data), size);
      os.close();
      return paths.emplace_back(std::move(path)).generic_string();
   }

   std::list<bfs::path> paths;
};

BOOST_AUTO_TEST_SUITE(compressed_file_tests)
BOOST_FIXTURE_TEST_CASE(random_access_test, temp_file_fixture) {
   // generate a large dataset where ever 8 bytes is the offset to that 8 bytes of data
   auto data = std::vector<uint64_t>(1024);
   std::generate(data.begin(), data.end(), [offset=0ULL]() mutable {
      auto result = offset;
      offset+=sizeof(uint64_t);
      return result;
   });

   auto uncompressed_filename = create_temp_file(data.data(), data.size() * sizeof(uint64_t));
   auto compressed_filename = create_temp_file(nullptr, 0);

   BOOST_TEST(compressed_file::process(uncompressed_filename, compressed_filename, 32));

   // test that you can read all of the offsets from the compressed form by opening and seeking to them
   for( const auto& entry : data ) {
      auto compf = compressed_file(compressed_filename);
      compf.open();
      compf.seek((long)entry);
      uint64_t value;
      compf.read(reinterpret_cast<char*>(&value), sizeof(value));
      BOOST_TEST(value == entry);
      compf.close();
   }
}

BOOST_FIXTURE_TEST_CASE(sequential_access, temp_file_fixture) {
   // generate a large dataset where ever 8 bytes is the offset to that 8 bytes of data
   auto data = std::vector<uint64_t>(1024);
   std::generate(data.begin(), data.end(), [offset=0ULL]() mutable {
      auto result = offset;
      offset+=sizeof(uint64_t);
      return result;
   });

   auto uncompressed_filename = create_temp_file(data.data(), data.size() * sizeof(uint64_t));
   auto compressed_filename = create_temp_file(nullptr, 0);

   BOOST_TEST(compressed_file::process(uncompressed_filename, compressed_filename, 32));

   // test that you can read all of the offsets from the compressed form sequentially
   auto compf = compressed_file(compressed_filename);
   compf.open();
   for( const auto& entry : data ) {
      uint64_t value;
      compf.read(reinterpret_cast<char*>(&value), sizeof(value));
      BOOST_TEST(value == entry);
   }
   compf.close();
}

BOOST_FIXTURE_TEST_CASE(blob_access, temp_file_fixture) {
   // generate a large dataset where ever 8 bytes is the offset to that 8 bytes of data
   auto data = std::vector<uint64_t>(1024);
   std::generate(data.begin(), data.end(), []() {
      return (uint64_t)std::rand();
   });

   auto uncompressed_filename = create_temp_file(data.data(), data.size() * sizeof(uint64_t));
   auto compressed_filename = create_temp_file(nullptr, 0);

   BOOST_TEST(compressed_file::process(uncompressed_filename, compressed_filename, 32));

   // test that you can read all of the offsets from the compressed form sequentially
   auto actual_data = std::vector<uint64_t>(1024);
   auto compf = compressed_file(compressed_filename);
   compf.open();
   compf.read(reinterpret_cast<char*>(actual_data.data()), actual_data.size() * sizeof(uint64_t));
   BOOST_TEST(data == actual_data, boost::test_tools::per_element());
   compf.close();
}


BOOST_AUTO_TEST_SUITE_END()