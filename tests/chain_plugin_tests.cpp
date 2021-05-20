#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <contracts.hpp>

#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

namespace std{
   std::ostream& operator << (std::ostream& s, fc::time_point tp){
      return s << (string)tp;
   }
}

std::string version_to_fixed_str(uint32_t ver){
   std::stringstream ss;
   ss << std::setfill('0') << std::setw(sizeof(uint32_t)<<1) << ver;
   return ss.str();
}



BOOST_AUTO_TEST_SUITE(chain_plugin_tests)

BOOST_FIXTURE_TEST_CASE( get_block_with_invalid_abi, TESTER ) try {
   produce_blocks(2);

   create_accounts( {"asserter"_n} );
   produce_block();

   // setup contract and abi
   set_code( "asserter"_n, contracts::asserter_wasm() );
   set_abi( "asserter"_n, contracts::asserter_abi().data() );
   produce_blocks(1);

   auto resolver = [&,this]( const account_name& name ) -> std::optional<abi_serializer> {
      try {
         const auto& accnt  = this->control->db().get<account_object,by_name>( name );
         abi_def abi;
         if (abi_serializer::to_abi(accnt.abi, abi)) {
            return abi_serializer(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));
         }
         return std::optional<abi_serializer>();
      } FC_RETHROW_EXCEPTIONS(error, "resolver failed at chain_plugin_tests::abi_invalid_type");
   };

   // abi should be resolved
   BOOST_REQUIRE_EQUAL(true, resolver("asserter"_n).has_value());

   // make an action using the valid contract & abi
   fc::variant pretty_trx = mutable_variant_object()
      ("actions", variants({
         mutable_variant_object()
            ("account", "asserter")
            ("name", "procassert")
            ("authorization", variants({
               mutable_variant_object()
                  ("actor", "asserter")
                  ("permission", name(config::active_name).to_string())
            }))
            ("data", mutable_variant_object()
               ("condition", 1)
               ("message", "Should Not Assert!")
            )
         })
      );
   signed_transaction trx;
   abi_serializer::from_variant(pretty_trx, trx, resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
   set_transaction_headers(trx);
   trx.sign( get_private_key( "asserter"_n, "active" ), control->get_chain_id() );
   push_transaction( trx );
   produce_blocks(1);

   // retrieve block num
   uint32_t headnum = this->control->head_block_num();

   char headnumstr[20];
   sprintf(headnumstr, "%d", headnum);
   chain_apis::read_only::get_block_params param{headnumstr};
   chain_apis::read_only plugin(*(this->control), {}, fc::microseconds::maximum());


   // block should be decoded successfully
   std::string block_str = json::to_pretty_string(plugin.get_block(param));
   BOOST_TEST(block_str.find("procassert") != std::string::npos);
   BOOST_TEST(block_str.find("condition") != std::string::npos);
   BOOST_TEST(block_str.find("Should Not Assert!") != std::string::npos);
   BOOST_TEST(block_str.find("011253686f756c64204e6f742041737365727421") != std::string::npos); //action data

   // set an invalid abi (int8->xxxx)
   std::string abi2 = contracts::asserter_abi().data();
   auto pos = abi2.find("int8");
   BOOST_TEST(pos != std::string::npos);
   abi2.replace(pos, 4, "xxxx");
   set_abi("asserter"_n, abi2.c_str());
   produce_blocks(1);

   // resolving the invalid abi result in exception
   BOOST_CHECK_THROW(resolver("asserter"_n), invalid_type_inside_abi);

   // get the same block as string, results in decode failed(invalid abi) but not exception
   std::string block_str2 = json::to_pretty_string(plugin.get_block(param));
   BOOST_TEST(block_str2.find("procassert") != std::string::npos);
   BOOST_TEST(block_str2.find("condition") == std::string::npos); // decode failed
   BOOST_TEST(block_str2.find("Should Not Assert!") == std::string::npos); // decode failed
   BOOST_TEST(block_str2.find("011253686f756c64204e6f742041737365727421") != std::string::npos); //action data

} FC_LOG_AND_RETHROW() /// get_block_with_invalid_abi

BOOST_FIXTURE_TEST_CASE( get_info, TESTER ) try {
   produce_blocks(1);

   chain_apis::read_only::get_info_params p;
   chain_apis::read_only plugin(*(this->control), {}, fc::microseconds::maximum());

   auto info = plugin.get_info({});
   BOOST_TEST(info.server_version == version_to_fixed_str(app().version()));
   BOOST_TEST(info.chain_id == control->get_chain_id());
   BOOST_TEST(info.head_block_num == control->head_block_num());
   BOOST_TEST(info.last_irreversible_block_num == control->last_irreversible_block_num());
   BOOST_TEST(info.last_irreversible_block_id == control->last_irreversible_block_id());
   BOOST_TEST(info.head_block_id == control->head_block_id());
   BOOST_TEST(info.head_block_time == control->head_block_time());
   BOOST_TEST(info.head_block_producer == control->head_block_producer());
   BOOST_TEST(info.virtual_block_cpu_limit == control->get_resource_limits_manager().get_virtual_block_cpu_limit());
   BOOST_TEST(info.virtual_block_net_limit == control->get_resource_limits_manager().get_virtual_block_net_limit());
   BOOST_TEST(info.block_cpu_limit == control->get_resource_limits_manager().get_block_cpu_limit());
   BOOST_TEST(info.block_net_limit == control->get_resource_limits_manager().get_block_net_limit());
   BOOST_TEST(*info.server_version_string == app().version_string());
   BOOST_TEST(*info.fork_db_head_block_num == control->fork_db_pending_head_block_num());
   BOOST_TEST(*info.fork_db_head_block_id == control->fork_db_pending_head_block_id());
   BOOST_TEST(*info.server_full_version_string == app().full_version_string());
   BOOST_TEST(*info.last_irreversible_block_time == control->last_irreversible_block_time());

   produce_blocks(1);

   //make sure it works after producing new block
   info = plugin.get_info({});
   BOOST_TEST(info.server_version == version_to_fixed_str(app().version()));
   BOOST_TEST(info.chain_id == control->get_chain_id());
   BOOST_TEST(info.head_block_num == control->head_block_num());
   BOOST_TEST(info.last_irreversible_block_num == control->last_irreversible_block_num());
   BOOST_TEST(info.last_irreversible_block_id == control->last_irreversible_block_id());
   BOOST_TEST(info.head_block_id == control->head_block_id());
   BOOST_TEST(info.head_block_time == control->head_block_time());
   BOOST_TEST(info.head_block_producer == control->head_block_producer());
   BOOST_TEST(info.virtual_block_cpu_limit == control->get_resource_limits_manager().get_virtual_block_cpu_limit());
   BOOST_TEST(info.virtual_block_net_limit == control->get_resource_limits_manager().get_virtual_block_net_limit());
   BOOST_TEST(info.block_cpu_limit == control->get_resource_limits_manager().get_block_cpu_limit());
   BOOST_TEST(info.block_net_limit == control->get_resource_limits_manager().get_block_net_limit());
   BOOST_TEST(*info.server_version_string == app().version_string());
   BOOST_TEST(*info.fork_db_head_block_num == control->fork_db_pending_head_block_num());
   BOOST_TEST(*info.fork_db_head_block_id == control->fork_db_pending_head_block_id());
   BOOST_TEST(*info.server_full_version_string == app().full_version_string());
   BOOST_TEST(*info.last_irreversible_block_time == control->last_irreversible_block_time());
} FC_LOG_AND_RETHROW() //get_info

BOOST_AUTO_TEST_SUITE_END()
