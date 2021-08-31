#include <sstream>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <contracts.hpp>
#include <snapshots.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

void remove_existing_blocks(controller::config& config) {
   auto block_log_path = config.blocks_dir / "blocks.log";
   remove(block_log_path);
   auto block_index_path = config.blocks_dir / "blocks.index";
   remove(block_index_path);
}

BOOST_AUTO_TEST_SUITE(restart_chain_tests)

BOOST_AUTO_TEST_CASE(test_existing_state_without_block_log)
{
   tester chain;

   std::vector<signed_block_ptr> blocks;
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());

   tester other;
   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
   blocks.clear();

   other.close();
   auto cfg = other.get_config();
   remove_existing_blocks(cfg);
   // restarting chain with no block log and no genesis
   other.open();

   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   chain.control->abort_block();

   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
}

BOOST_AUTO_TEST_CASE(test_restart_with_different_chain_id)
{
   tester chain;

   std::vector<signed_block_ptr> blocks;
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());

   tester other;
   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
   blocks.clear();

   other.close();
   genesis_state genesis;
   genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:01.000");
   genesis.initial_key = eosio::testing::base_tester::get_public_key( config::system_account_name, "active" );
   fc::optional<chain_id_type> chain_id = genesis.compute_chain_id();
   BOOST_REQUIRE_EXCEPTION(other.open(chain_id), chain_id_type_exception, fc_exception_message_starts_with("chain ID in state "));
}

namespace{
   struct scoped_temp_path {
      boost::filesystem::path path;
      scoped_temp_path() {
         path = boost::filesystem::unique_path();
         if (boost::unit_test::framework::master_test_suite().argc >= 2) {
            path += boost::unit_test::framework::master_test_suite().argv[1];
         }
      }
      ~scoped_temp_path() {
         boost::filesystem::remove_all(path);
      }
   };
}

enum class buf_len_type { small, medium, large };

void trim_blocklog_front(uint32_t truncate_at_block, buf_len_type len_type) {
   tester chain;
   chain.produce_blocks(30);
   chain.close();

   namespace bfs = boost::filesystem;

   auto  blocks_dir     = chain.get_config().blocks_dir;
   scoped_temp_path temp1, temp2;
   boost::filesystem::create_directory(temp1.path);
   bfs::copy(blocks_dir / "blocks.log", temp1.path / "blocks.log");
   bfs::copy(blocks_dir / "blocks.index", temp1.path / "blocks.index");

   trim_data td(temp1.path);
   uint64_t blk_size = td.block_pos(30) - td.block_pos(29);
   uint64_t log_size = td.block_pos(30) + blk_size;

   switch (len_type){
      case buf_len_type::small:
         block_log::set_buff_len( blk_size + (sizeof(uint64_t) - 1));
         break;
      case buf_len_type::medium:
         block_log::set_buff_len( log_size / 3);
         break;
      case buf_len_type::large:
         block_log::set_buff_len(log_size);
         break;
      default:
         return;
   }

   BOOST_CHECK( block_log::trim_blocklog_front(temp1.path, temp2.path, truncate_at_block) == true);
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front) {
   trim_blocklog_front(5, buf_len_type::small);
   trim_blocklog_front(6, buf_len_type::small);
   trim_blocklog_front(10, buf_len_type::medium);
   trim_blocklog_front(11, buf_len_type::medium);
   trim_blocklog_front(15, buf_len_type::large);
   trim_blocklog_front(16, buf_len_type::large);
}

BOOST_AUTO_TEST_SUITE_END()
