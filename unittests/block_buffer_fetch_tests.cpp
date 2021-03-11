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

    void fetch_block_buffer(TESTER& chain, uint32_t block_num, bool return_signed_block){
        std::shared_ptr<std::vector<char>> pBuffer;

        pBuffer = chain.control->fetch_block_buffer_by_number(block_num, return_signed_block);
        if (!pBuffer) return;

        //unpack
        fc::datastream<const char*> ds((*pBuffer).data(), (*pBuffer).size());
        if (return_signed_block){
            signed_block sb;
            sb.unpack(ds, packed_transaction::cf_compression_type::none);
            BOOST_CHECK(sb.block_num() == block_num);
        }else{
            signed_block_v0 sb_v0;
            fc::raw::unpack(ds, sb_v0);
            BOOST_CHECK(sb_v0.block_num() == block_num);
        }
    }

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

            //fetch a block that is in a catalog file
            fetch_block_buffer(chain, 60, true);
            fetch_block_buffer(chain, 60, false);
            //fetch a block that is in the blocks.log file
            fetch_block_buffer(chain, 145, true);
            fetch_block_buffer(chain, 145, false);
            //fetch a block that doesn't exist locally
            fetch_block_buffer(chain, 200, true);
            fetch_block_buffer(chain, 200, false);
        }FC_LOG_AND_RETHROW()

    }

    struct blocklog_version_set {
        blocklog_version_set(uint32_t ver) { block_log::set_version(ver); };
        ~blocklog_version_set() { block_log::set_version(block_log::max_supported_version); };
    };

    BOOST_AUTO_TEST_CASE(fetch_signed_block_v0_test) {
        try{
            namespace bfs = boost::filesystem;
            fc::temp_directory temp_dir;

            // set block log version less than block_log::max_supported_version
            blocklog_version_set set_version(1);

            TESTER chain(
                    temp_dir,
                    [](controller::config& config) {
                        config.blog.archive_dir        = "archive";
                        config.blog.stride             = 20;
                        config.blog.max_retained_files = 5;
                    },
                    true);

            //produce less than stride size (20) blocks to avoid block-log-split that sets log version as block_log::max_supported_version
            chain.produce_blocks(10);

            auto blocks_dir = chain.get_config().blog.log_dir;
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks.log" ));
            BOOST_CHECK(bfs::exists( blocks_dir / "blocks.index" ));

            //fetch a block that is in blocks.log file
            fetch_block_buffer(chain, 5, true);
            fetch_block_buffer(chain, 5, false);
            //fetch the head block
            fetch_block_buffer(chain, 10, true);
            fetch_block_buffer(chain, 10, false);
            //fetch a block that doesn't exist locally
            fetch_block_buffer(chain, 20, true);
            fetch_block_buffer(chain, 20, false);

        }FC_LOG_AND_RETHROW()
    }

BOOST_AUTO_TEST_SUITE_END()