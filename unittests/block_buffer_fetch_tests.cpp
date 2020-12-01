#include <sstream>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <fc/io/cfile.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace testing;
using namespace chain;
using namespace eosio::chain;
using namespace eosio::testing;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(block_buffer_fetch_tests)

    BOOST_AUTO_TEST_CASE(fetch_signed_block_test) {
        try{
            namespace bfs = boost::filesystem;
            fc::temp_directory temp_dir;

            TESTER chain(
                    temp_dir,
                    [](controller::config& config) {
                        config.blog.archive_dir        = "archive";
                        config.blog.stride             = 20;
                        config.blog.max_retained_files = 5;
                    },
                    true);
            chain.produce_blocks(150);

            auto blocks_dir = chain.get_config().blog.log_dir;
            auto blocks_archive_dir = blocks_dir / chain.get_config().blog.archive_dir;

            BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-1-20.log" ));
            BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-1-20.index" ));
            BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-21-40.log" ));
            BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-21-40.index" ));

            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-60.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-60.index" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-80.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-80.index" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-81-100.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-81-100.index" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-101-120.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-101-120.index" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-121-140.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks-121-140.index" ));

            std::shared_ptr<std::vector<char>> pBuffer;
            pBuffer = chain.control->fetch_block_buffer_by_number(60,true);
            //unpack
            fc::datastream<const char*> ds((*pBuffer).data(), (*pBuffer).size());
            signed_block sb;
            sb.unpack(ds, packed_transaction::cf_compression_type::none);

            BOOST_CHECK( sb.block_num() == 60);

            //...

        }FC_LOG_AND_RETHROW()

    }

    BOOST_AUTO_TEST_CASE(fetch_signed_block_v0_test) {
        try{
            TESTER test;
            std::vector<signed_block_ptr> blocks;

            blocks.push_back(test.produce_block());
            blocks.push_back(test.produce_block());
            blocks.push_back(test.produce_block());

            //convert to signed_block_v0
            signed_block_ptr sbp = blocks.back();
            auto signed_block_v0 = sbp->to_signed_block_v0();


            //...

        }FC_LOG_AND_RETHROW()
    }

BOOST_AUTO_TEST_SUITE_END()