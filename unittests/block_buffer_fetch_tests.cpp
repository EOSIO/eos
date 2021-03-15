#include <sstream>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <fc/io/cfile.hpp>
#include <boost/asio.hpp>
#include <ctime>

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

    uint32_t fetch_block_num(TESTER& chain, uint32_t block_num, bool return_signed_block){
        std::shared_ptr<std::vector<char>> pBuffer;

        pBuffer = chain.control->fetch_block_buffer_by_number(block_num, return_signed_block);
        if (!pBuffer)
            return block_num +1;

        //unpack
        fc::datastream<const char*> ds((*pBuffer).data(), (*pBuffer).size());
        if (return_signed_block){
            signed_block sb;
            sb.unpack(ds, packed_transaction::cf_compression_type::none);
            return sb.block_num();
        }else{
            signed_block_v0 sb_v0;
            fc::raw::unpack(ds, sb_v0);
            return sb_v0.block_num();
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

            auto head_num = chain.control->head_block_num();

            //fetch a block that is in a catalog file
            BOOST_CHECK(fetch_block_num(chain, 60, true) == 60);
            BOOST_CHECK(fetch_block_num(chain, 60, false) == 60);
            //fetch a block that is in the blocks.log file
            BOOST_CHECK(fetch_block_num(chain, 145, true) == 145);
            BOOST_CHECK(fetch_block_num(chain, 145, false) == 145);
            //fetch a block that doesn't exist locally
            BOOST_CHECK(fetch_block_num(chain, 200, true) != 200);
            BOOST_CHECK(fetch_block_num(chain, 200, false) != 200);
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
            BOOST_CHECK(fetch_block_num(chain, 5, true) == 5);
            BOOST_CHECK(fetch_block_num(chain, 5, false) == 5);
            //fetch the head block
            BOOST_CHECK(fetch_block_num(chain, 10, true) == 10);
            BOOST_CHECK(fetch_block_num(chain, 10, false) == 10);
            //fetch a block that doesn't exist locally
            BOOST_CHECK(fetch_block_num(chain, 20, true) != 20);
            BOOST_CHECK(fetch_block_num(chain, 20, false) != 20);

        }FC_LOG_AND_RETHROW()
    }

    void write_blocks(TESTER &chain){
        chain.produce_blocks(10);
    }

    void read_block(TESTER& chain, uint32_t search_block_num, bool return_signed_block){
        try{
            uint32_t result_num = fetch_block_num(chain, search_block_num, return_signed_block);
            if (result_num != search_block_num){
                throw;
            }
        }FC_LOG_AND_RETHROW()
    }
    BOOST_AUTO_TEST_CASE(multi_threaded_test) {
        try {
            auto io_serv = std::make_shared<boost::asio::io_service>();
            auto work_ptr = std::make_unique<boost::asio::io_service::work>(*io_serv);

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

            constexpr int num_threads = 5;
            std::array<std::thread, num_threads> threads;;
            for (auto i = 0; i < num_threads; ++i)
            {
                threads[i] = std::thread([&io_serv]() {
                    io_serv->run();
                });
            }

            boost::asio::post(*io_serv,  std::bind(write_blocks, std::ref(chain)));
            while (chain.control->head_block_num() <= 1)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

            for (auto i  = 0; i < num_threads*2; ++i){
                //boost::asio::post(*io_serv,  std::bind(write_blocks, std::ref(chain))); // Assertion failed on https://github.com/EOSIO/eos/blob/epe-437-retrieve-blocks-during-sync/libraries/chain/controller.cpp#L1324

                std::srand(std::time(nullptr));   // use current time as seed for random generator
                uint32_t search_block_num;
                do{
                    search_block_num = (std::rand() % chain.control->head_block_num())+1;
                }while (search_block_num <= 0);
                bool return_signed_block = std::rand() % 2;
                boost::asio::post(*io_serv,  std::bind(read_block, std::ref(chain), search_block_num, return_signed_block));
            }

            work_ptr.reset();
            for (auto & t : threads)
            {
                t.join();
            }

        }FC_LOG_AND_RETHROW()
    }
/*
    BOOST_AUTO_TEST_CASE(multi_threaded_test2) {
        try {
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

             for (auto i  = 0; i < 10; ++i){
                boost::asio::post(chain.control->get_thread_pool().get_executor(), std::bind(write_blocks, std::ref(chain))); // Assertion failed on https://github.com/EOSIO/eos/blob/epe-437-retrieve-blocks-during-sync/libraries/chain/controller.cpp#L1324

                std::srand(std::time(nullptr));   // use current time as seed for random generator
                uint32_t search_block_num;
                do{
                    search_block_num = (std::rand() % chain.control->head_block_num())+1;
                }while (search_block_num <= 0);
                bool return_signed_block = std::rand() % 2;
                boost::asio::post(chain.control->get_thread_pool().get_executor(), std::bind(read_block, std::ref(chain), search_block_num, return_signed_block));
            }
        }FC_LOG_AND_RETHROW()
    }
*/

BOOST_AUTO_TEST_SUITE_END()