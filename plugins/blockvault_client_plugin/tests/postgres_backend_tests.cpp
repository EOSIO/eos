#include "../postgres_backend.hpp"
#include <boost/test/unit_test.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/appender.hpp>

std::vector<char> mock_snapshot_content = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k'};

// create a test block_id where the first 4 bytes is block_num and the 5th byte
// is discriminator, the purpose of discriminator is to differentiate block ids with
// the same block_num
struct mock_block_id {
   char buf[32];
   mock_block_id(uint32_t block_num, char discriminator) {
      memset(buf, 0, 32);
      memcpy(buf, &block_num, sizeof(block_num));
      buf[4] = discriminator;
   }

   operator std::string_view() const { return std::string_view(buf, 32); }
};

template <typename Backend>
struct backend_reset;

template <>
struct backend_reset<eosio::blockvault::postgres_backend> {
   backend_reset() {
      pqxx::connection conn;
      pqxx::work       w(conn);
      w.exec("DROP TABLE IF EXISTS BlockData; DROP TABLE IF EXISTS SnapshotData; SELECT lo_unlink(l.oid) FROM "
             "pg_largeobject_metadata l;");
      w.commit();
   }
};

template <typename Backend>
struct backend_test_fixture : backend_reset<Backend> {

   Backend       backend;
   fc::temp_file tmp_file;

   backend_test_fixture()
       : backend("") {

      std::filebuf fbuf;
      fbuf.open(tmp_file.path().string().c_str(), std::ios::out | std::ios::binary);
      fbuf.sputn(mock_snapshot_content.data(), mock_snapshot_content.size());
   }

   bool propose_constructed_block(uint32_t watermark_bn, uint32_t watermark_ts, uint32_t lib,
                                  char block_discriminator = 'a', char previous_block_discriminator = 'b') {
      std::vector<char> block(64);
      block[0] = block_discriminator;
      return backend.propose_constructed_block({watermark_bn, watermark_ts}, lib, block,
                                               mock_block_id(watermark_bn, block_discriminator),
                                               mock_block_id(watermark_bn - 1, previous_block_discriminator));
   }

   bool append_external_block(uint32_t block_num, uint32_t lib, char block_discriminator = 'a',
                              char previous_block_discriminator = 'b') {
      std::vector<char> block(64);
      block[0] = block_discriminator;

      return backend.append_external_block(block_num, lib, block, mock_block_id{block_num, block_discriminator},
                                           mock_block_id{block_num - 1, previous_block_discriminator});
   }

   bool propose_snapshot(uint32_t watermark_bn, uint32_t watermark_ts) {
      return backend.propose_snapshot({watermark_bn, watermark_ts}, tmp_file.path().string().c_str());
   }

   void sync(std::optional<mock_block_id> previous_block_id, eosio::blockvault::backend::sync_callback& callback) {
      if (previous_block_id) {
         backend.sync(*previous_block_id, callback);
      } else
         backend.sync({0, 0}, callback);
   }
};

typedef boost::mpl::list<eosio::blockvault::postgres_backend> test_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_propose_constructed_block, T, test_types) {
   backend_test_fixture<T> fixture;

   // watermark_bn and watermark_ts should be increasing,
   // lib should be non-decreasing

   BOOST_REQUIRE(fixture.propose_constructed_block(10, 1, 1, 'a'));
   BOOST_REQUIRE(fixture.propose_constructed_block(11, 2, 2, 'b'));
   BOOST_REQUIRE(fixture.propose_constructed_block(12, 3, 3, 'c'));

   // watermark_ts is  not increasing
   BOOST_REQUIRE(!fixture.propose_constructed_block(13, 3, 4, 'd'));

   // watermark_bn is not increasing
   BOOST_REQUIRE(!fixture.propose_constructed_block(12, 4, 4, 'd'));

   // lib is not increasing
   BOOST_REQUIRE(fixture.propose_constructed_block(13, 4, 3, 'd'));

   // watermark_bn, watermark_ts and lib are all increasing
   BOOST_REQUIRE(fixture.propose_constructed_block(14, 5, 4, 'e'));

   // lib is decreasing
   BOOST_REQUIRE(!fixture.propose_constructed_block(15, 6, 3, 'f'));

   // watermark_bn is decreasing
   BOOST_REQUIRE(!fixture.propose_constructed_block(13, 6, 4, 'f'));

   // watermark_ts is decreasing
   BOOST_REQUIRE(!fixture.propose_constructed_block(15, 4, 4, 'f'));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_append_external_block, T, test_types) {

   backend_test_fixture<T> fixture;

   BOOST_REQUIRE(fixture.append_external_block(9, 1));

   BOOST_REQUIRE(fixture.propose_constructed_block(10, 1, 2));
   BOOST_REQUIRE(fixture.propose_constructed_block(11, 2, 3));
   BOOST_REQUIRE(fixture.propose_constructed_block(12, 3, 4)); // watermark (12, 3), max_lib 4

   // block.block_num > max_lib
   BOOST_REQUIRE(fixture.append_external_block(5, 4));

   // block.block_num == max_lib
   BOOST_REQUIRE(!fixture.append_external_block(4, 4));

   // block.block_num < max_lib
   BOOST_REQUIRE(!fixture.append_external_block(2, 4));

   // block lib > max_lib
   BOOST_REQUIRE(fixture.append_external_block(6, 5));

   BOOST_REQUIRE(fixture.propose_constructed_block(13, 4, 5));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_propose_snapshot, T, test_types) {

   backend_test_fixture<T> fixture;

   BOOST_REQUIRE(fixture.propose_snapshot(10, 1));

   // increase both watermark block_num & timestamp
   BOOST_REQUIRE(fixture.propose_snapshot(11, 2));

   // just increase  watermark timestamp
   BOOST_REQUIRE(!fixture.propose_snapshot(11, 3));

   // just increase watermark num
   BOOST_REQUIRE(!fixture.propose_snapshot(12, 2));
}

struct mock_sync_callback : eosio::blockvault::backend::sync_callback {

   std::vector<char> expected_snapshot_content;
   std::vector<char> reverse_expected_block_first_bytes;
   std::vector<char> received_snapshot_content;

   void on_snapshot(const char* snapshot_filename) override {
      std::ifstream               is(snapshot_filename);
      std::istream_iterator<char> start(is), end;
      std::copy(start, end, std::back_inserter(received_snapshot_content));
   }

   void on_block(std::string_view block) override {
      BOOST_REQUIRE(reverse_expected_block_first_bytes.size());
      BOOST_CHECK_EQUAL(block[0], reverse_expected_block_first_bytes.back());
      reverse_expected_block_first_bytes.pop_back();
   }

   ~mock_sync_callback() {
      BOOST_CHECK(std::equal(expected_snapshot_content.begin(), expected_snapshot_content.end(),
                             received_snapshot_content.begin()));
      BOOST_CHECK(reverse_expected_block_first_bytes.empty());
   }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(test_sync, T, test_types) {

   backend_test_fixture<T> fixture;

   BOOST_REQUIRE(fixture.propose_constructed_block(10, 1, 1, 'a', '\0')); // watermark (10, 1), max_lib 1
   BOOST_REQUIRE(fixture.propose_constructed_block(11, 2, 2, 'b', 'a'));  // watermark (11, 2), max_lib 2
   BOOST_REQUIRE(fixture.propose_constructed_block(12, 3, 3, 'c', 'b'));  // watermark (12, 3), max_lib 3
   BOOST_REQUIRE(fixture.propose_constructed_block(13, 4, 4, 'd', 'c'));  // watermark (13, 4), max_lib 4

   // append two different blocks with the same block numbers
   BOOST_REQUIRE(fixture.append_external_block(13, 4, 'e', 'a')); // watermark (13, 4), max_lib 4
   BOOST_REQUIRE(fixture.append_external_block(13, 4, 'f', 'a')); // watermark (13, 4), max_lib 4

   BOOST_REQUIRE(fixture.propose_constructed_block(14, 5, 5, 'g', 'd')); // watermark (14, 5), max_lib 5

   //  block relationship constructed by above code
   //
   //  (10,1)   (11,2)  (12,3) (13,4) (14,5)
   //
   //         +-- b ---- c ---- d ---- g
   //    a  --+---------------- e
   //         +---------------- f
   //

   {
      mock_sync_callback callback;
      callback.reverse_expected_block_first_bytes = {'g', 'f', 'e', 'd', 'c', 'b', 'a'};
      BOOST_REQUIRE_NO_THROW(fixture.sync(std::optional<mock_block_id>{}, callback));
   }

   {
      mock_sync_callback callback;
      callback.reverse_expected_block_first_bytes = {'g', 'f', 'e', 'd'};
      BOOST_REQUIRE_NO_THROW(fixture.sync(mock_block_id(12, 'c'), callback));
   }

   {
      mock_sync_callback callback;
      callback.reverse_expected_block_first_bytes = {'g', 'f', 'e', 'd', 'c', 'b'};

      // given the previous_block_id is {4, 'a'} , there are 3 blocks have the same previous_block_id
      // (i.e. block 'b' and 'e', and 'f'), we should sync every block from the lower waterwark, (i.e. block 'b' )
      BOOST_REQUIRE_NO_THROW(fixture.sync(mock_block_id(10, 'a'), callback));
   }

   BOOST_REQUIRE(fixture.propose_snapshot(12, 1));

   {
      mock_sync_callback callback;
      callback.expected_snapshot_content          = mock_snapshot_content;
      callback.reverse_expected_block_first_bytes = {'g', 'f', 'e', 'd'};
      // sync from empty block id
      BOOST_REQUIRE_NO_THROW(fixture.sync(std::optional<mock_block_id>{}, callback));
   }

   {
      mock_sync_callback callback;
      callback.expected_snapshot_content          = mock_snapshot_content;
      callback.reverse_expected_block_first_bytes = {'g', 'f', 'e', 'd'};
      // sync from non-existant block id
      BOOST_REQUIRE_NO_THROW(fixture.sync(mock_block_id{12, 't'}, callback));
   }

   {
      mock_sync_callback callback;
      callback.reverse_expected_block_first_bytes = {'g'};
      // sync from existant block id
      BOOST_REQUIRE_NO_THROW(fixture.sync(mock_block_id{13, 'd'}, callback));
   }

   {
      mock_sync_callback callback;
      // sync from the most recent block id
      BOOST_REQUIRE_NO_THROW(fixture.sync(mock_block_id{14, 'g'}, callback));
   }
}
