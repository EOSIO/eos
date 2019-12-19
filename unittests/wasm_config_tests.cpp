#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/string.hpp>
#include <fc/variant_object.hpp>

#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/unit_test.hpp>

#include "test_wasts.hpp"

#include <contracts.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
namespace data = boost::unit_test::data;

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

namespace {
struct wasm_config_tester : TESTER {
   wasm_config_tester() {
      set_abi(config::system_account_name, contracts::wasm_config_bios_abi().data());
      set_code(config::system_account_name, contracts::wasm_config_bios_wasm());
      bios_abi_ser = *get_resolver()(config::system_account_name);
   }
   void set_wasm_params(const wasm_config& params) {
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(eosio),config::active_name}}, N(eosio), N(setwparams),
                               bios_abi_ser.variant_to_binary("setwparams", fc::mutable_variant_object()("cfg", params), abi_serializer_max_time));
      trx.actions[0].authorization = vector<permission_level>{{N(eosio),config::active_name}};
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(eosio), "active"), control->get_chain_id());
      push_transaction(trx);
   }
   void push_action(account_name account) {
       signed_transaction trx;
       trx.actions.push_back({{{account,config::active_name}}, account, name(), {}});
       set_transaction_headers(trx);
       trx.sign(get_private_key( account, "active" ), control->get_chain_id());
       push_transaction(trx);
   }
   chain::abi_serializer bios_abi_ser;
};
struct old_wasm_tester : tester {
   old_wasm_tester() : tester{setup_policy::old_wasm_parser} {}
};

std::string make_locals_wasm(int n_params, int n_locals, int n_stack)
{
   std::stringstream ss;
   ss << "(module ";
   ss << " (func (export \"apply\") (param i64 i64 i64))";
   ss << " (func ";
   for(int i = 0; i < n_params; i+=8)
      ss << "(param i64)";
   for(int i = 0; i < n_locals; i+=8)
      ss << "(local i64)";
   for(int i = 0; i < n_stack; i+=8)
      ss << "(i64.const 0)";
   for(int i = 0; i < n_stack; i+=8)
      ss << "(drop)";
   ss << " )";
   ss << ")";
   return ss.str();
}

}

BOOST_AUTO_TEST_SUITE(wasm_config_tests)

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_mutable_global_bytes, data::make({ 4096, 8192 , 16384 }) * data::make({0, 1}), n_globals, oversize) {
   produce_block();
   create_accounts({N(globals)});
   produce_block();

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_mutable_global_bytes = n_globals;
   set_wasm_params(params);

   std::string code = [&] {
      std::ostringstream ss;
      ss << "(module ";
      ss << " (func $eosio_assert (import \"env\" \"eosio_assert\") (param i32 i32))";
      ss << " (memory 1)";
      for(int i = 0; i < n_globals + oversize; i += 4)
         ss << "(global (mut i32) (i32.const " << i << "))";
      ss << " (func (export \"apply\") (param i64 i64 i64)";
      for(int i = 0; i < n_globals + oversize; i += 4)
         ss << "(call $eosio_assert (i32.eq (get_global " << i/4 << ") (i32.const " << i << ")) (i32.const 0))";
      ss << " )";
      ss << ")";
      return ss.str();
   }();

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(globals), code.c_str()), wasm_exception);
      produce_block();
   } else {
      set_code(N(globals), code.c_str());
      push_action(N(globals));
      produce_block();
      --params.max_mutable_global_bytes;
      set_wasm_params(params);
      push_action(N(globals));
   }
}

static const std::vector<std::tuple<int, int, bool, bool>> func_local_params = {
   // Default value of max_func_local_bytes
   {8192, 0, false, true}, {4096, 4096, false, true}, {0, 8192, false, true},
   {8192 + 1, 0, false, false}, {4096 + 1, 4096, false, false}, {0, 8192 + 1, false, false},
   // Larger than the default
   {16384, 0, true, true}, {8192, 8192, true, true}, {0, 16384, true, true},
   {16384 + 1, 0, true, false}, {8192 + 1, 8192, true, false}, {0, 16384 + 1, true, false}
};

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_func_local_bytes, data::make({0, 8192, 16384}) * data::make(func_local_params), n_params, n_locals, n_stack, set_high, expect_success) {
   produce_blocks(2);
   create_accounts({N(stackz)});
   produce_block();

   auto def_params = genesis_state::default_initial_wasm_configuration;
   auto high_params = def_params;
   high_params.max_func_local_bytes = 16384;
   auto low_params = def_params;
   low_params.max_func_local_bytes = 4096;

   if(set_high) {
      set_wasm_params(high_params);
      produce_block();
   }

   auto pushit = [&]() {
      action act;
      act.account = N(stackz);
      act.name = name();
      act.authorization = vector<permission_level>{{N(stackz),config::active_name}};
      signed_transaction trx;
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(stackz), "active" ), control->get_chain_id());
      push_transaction(trx);
   };

   std::string code = make_locals_wasm(n_params, n_locals, n_stack);

   if(expect_success) {
      set_code(N(stackz), code.c_str());
      produce_block();
      pushit();
      set_wasm_params(low_params);
      produce_block();
      pushit(); // Only checked at set_code.
      set_code(N(stackz), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(stackz), code.c_str()), wasm_exception);
      produce_block();
   } else {
      BOOST_CHECK_THROW(set_code(N(stackz), code.c_str()), wasm_exception);
      produce_block();
   }
}

BOOST_FIXTURE_TEST_CASE(max_func_local_bytes_mixed, wasm_config_tester) {
   produce_blocks(2);
   create_accounts({N(stackz)});
   produce_block();

   std::string code;
   {
      std::stringstream ss;
      ss << "(module ";
      ss << " (func (export \"apply\") (param i64 i64 i64))";
      ss << " (func ";
      ss << "   (local i32 i64 i64 f32 f32 f32 f32 f64 f64 f64 f64 f64 f64 f64 f64)";
      for(int i = 0; i < 16; ++i)
         ss << "(i32.const 0)";
      for(int i = 0; i < 32; ++i)
         ss << "(i64.const 0)";
      for(int i = 0; i < 64; ++i)
         ss << "(f32.const 0)";
      for(int i = 0; i < 128; ++i)
         ss << "(f64.const 0)";
      ss << "(drop)(f64.const 0)";
      ss << "(return)";
      for(int i = 0; i < 8192; ++i)
         ss << "(i64.const 0)";
      ss << "(return)";
      ss << " )";
      ss << ")";
      code = ss.str();
   }
   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_func_local_bytes = 4 + 16 + 16 + 64 + 64 + 256 + 256 + 1024;
   set_code(N(stackz), code.c_str());
   set_code(N(stackz), std::vector<uint8_t>{});
   produce_block();
   --params.max_func_local_bytes;
   set_wasm_params(params);
   BOOST_CHECK_THROW(set_code(N(stackz), code.c_str()), wasm_exception);
}

static const std::vector<std::tuple<int, int, bool>> old_func_local_params = {
   {8192, 0, true}, {4096, 4096, true}, {0, 8192, true},
   {8192 + 1, 0, false}, {4096 + 1, 4096, false}, {0, 8192 + 1, false},
};

BOOST_DATA_TEST_CASE_F(old_wasm_tester, max_func_local_bytes_old, data::make({0, 8192, 16384}) * data::make(old_func_local_params), n_stack, n_params, n_locals, expect_success) {
   produce_blocks(2);
   create_accounts({N(stackz)});
   produce_block();

   auto pushit = [&]() {
      action act;
      act.account = N(stackz);
      act.name = name();
      act.authorization = vector<permission_level>{{N(stackz),config::active_name}};
      signed_transaction trx;
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(stackz), "active" ), control->get_chain_id());
      push_transaction(trx);
   };

   std::string code = make_locals_wasm(n_params, n_locals, n_stack);

   if(expect_success) {
      set_code(N(stackz), code.c_str());
      produce_block();
      pushit();
      produce_block();
   } else {
      BOOST_CHECK_THROW(set_code(N(stackz), code.c_str()), wasm_exception);
      produce_block();
   }
}

BOOST_FIXTURE_TEST_CASE(max_func_local_bytes_mixed_old, old_wasm_tester) {
   produce_blocks(2);
   create_accounts({N(stackz)});
   produce_block();

   std::string code;
   {
      std::stringstream ss;
      ss << "(module ";
      ss << " (func (export \"apply\") (param i64 i64 i64))";
      ss << " (func ";
      ss << "   (param i32 i64 i64 f32 f32 f32 f32 f64 f64 f64 f64 f64 f64 f64 f64)";
      for(int i = 0; i < 16; ++i)
         ss << "(local i32)";
      for(int i = 0; i < 32; ++i)
         ss << "(local i64)";
      for(int i = 0; i < 64; ++i)
         ss << "(local f32)";
      for(int i = 0; i < 128; ++i)
         ss << "(local f64)";
      // pad to 8192 bytes
      for(int i = 0; i < 1623; ++i)
         ss << "(local i32)";
      ss << " )";
      ss << ")";
      code = ss.str();
   }
   set_code(N(stackz), code.c_str());
   produce_block();
   // add one more parameter
   code.replace(code.find("param i32"), 5, "param f32");
   BOOST_CHECK_THROW(set_code(N(stackz), code.c_str()), wasm_exception);
}

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_table_elements, data::make({512, 2048}) * data::make({0, 1}), max_table_elements, oversize) {
   produce_block();
   create_accounts( { N(table) } );
   produce_block();

   auto pushit = [&]{
      signed_transaction trx;
      trx.actions.push_back({{{N(table),config::active_name}}, N(table), name(), {}});
      set_transaction_headers(trx);
      trx.sign(get_private_key( N(table), "active" ), control->get_chain_id());
      push_transaction(trx);
   };

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_table_elements = max_table_elements;
   set_wasm_params(params);

   std::string code = fc::format_string(variable_table, fc::mutable_variant_object()("TABLE_SIZE", max_table_elements + oversize)("TABLE_OFFSET", max_table_elements - 2));
   if(!oversize) {
      set_code(N(table), code.c_str());
      pushit();
      produce_block();

      --params.max_table_elements;
      set_wasm_params(params);
      pushit();
   } else {
      BOOST_CHECK_THROW(set_code(N(table), code.c_str()), wasm_exception);
   }
}

BOOST_FIXTURE_TEST_CASE( max_pages, wasm_config_tester ) try {
   produce_blocks(2);

   create_accounts( {N(bigmem)} );
   produce_block();
   auto params = genesis_state::default_initial_wasm_configuration;
   for(uint64_t max_pages : {600, 400}) { // above and below the default limit
      params.max_pages = max_pages;
      set_wasm_params(params);
      produce_block();

      string biggest_memory_wast_f = fc::format_string(biggest_memory_variable_wast, fc::mutable_variant_object(
                                                       "MAX_WASM_PAGES", params.max_pages - 1));

      set_code(N(bigmem), biggest_memory_wast_f.c_str());
      produce_blocks(1);

      auto pushit = [&](uint64_t extra_pages) {
         action act;
         act.account = N(bigmem);
         act.name = name(extra_pages);
         act.authorization = vector<permission_level>{{N(bigmem),config::active_name}};
         signed_transaction trx;
         trx.actions.push_back(act);

         set_transaction_headers(trx);
         trx.sign(get_private_key( N(bigmem), "active" ), control->get_chain_id());
         //but should not be able to grow beyond largest page
         push_transaction(trx);
      };

      pushit(1);
      produce_blocks(1);

      // Increase memory limit
      ++params.max_pages;
      set_wasm_params(params);
      produce_block();
      pushit(2);

      // Decrease memory limit
      params.max_pages -= 2;
      set_wasm_params(params);
      produce_block();
      pushit(0);

      // Reduce memory limit below initial memory
      --params.max_pages;
      set_wasm_params(params);
      produce_block();
      BOOST_CHECK_THROW(pushit(0), eosio::chain::wasm_exception);

      params.max_pages = max_pages;
      set_wasm_params(params);
      string too_big_memory_wast_f = fc::format_string(too_big_memory_wast, fc::mutable_variant_object(
                                                       "MAX_WASM_PAGES_PLUS_ONE", params.max_pages+1));
      BOOST_CHECK_THROW(set_code(N(bigmem), too_big_memory_wast_f.c_str()), eosio::chain::wasm_exception);

      // Check that the max memory defined by the contract is respected
      string memory_over_max_wast = fc::format_string(max_memory_wast, fc::mutable_variant_object()
                                                      ("INIT_WASM_PAGES", params.max_pages - 3)
                                                      ("MAX_WASM_PAGES", params.max_pages - 1));
      set_code(N(bigmem), memory_over_max_wast.c_str());
      produce_block();
      pushit(2);

      // Move max_pages in between the contract's initial and maximum memories
      params.max_pages -= 2;
      set_wasm_params(params);
      produce_block();
      pushit(1);

      // Move it back
      params.max_pages += 2;
      set_wasm_params(params);
      produce_block();
      pushit(2);
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( call_depth, wasm_config_tester ) try {
   produce_block();
   create_accounts( {N(depth)} );
   produce_block();

   uint32_t max_call_depth = 150;
   auto high_params = genesis_state::default_initial_wasm_configuration;
   high_params.max_call_depth = max_call_depth + 1;
   wasm_config low_params = high_params;
   low_params.max_call_depth = 50;
   set_wasm_params(high_params);
   produce_block();

   signed_transaction trx;
   trx.actions.emplace_back(vector<permission_level>{{N(depth),config::active_name}}, N(depth), N(), bytes{});
   trx.actions[0].authorization = vector<permission_level>{{N(depth),config::active_name}};

   auto pushit = [&]() {
      produce_block();
      trx.signatures.clear();
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(depth), "active"), control->get_chain_id());
      push_transaction(trx);
   };

   //strictly wasm recursion to maximum_call_depth & maximum_call_depth+1
   string wasm_depth_okay = fc::format_string(depth_assert_wasm, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth));
   set_code(N(depth), wasm_depth_okay.c_str());
   pushit();

   // The depth should not be cached.
   set_wasm_params(low_params);
   BOOST_CHECK_THROW(pushit(), wasm_execution_error);
   set_wasm_params(high_params);
   produce_block();
   pushit();
   produce_block();

   string wasm_depth_one_over = fc::format_string(depth_assert_wasm, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth+1));
   set_code(N(depth), wasm_depth_one_over.c_str());
   BOOST_CHECK_THROW(pushit(), wasm_execution_error);

   //wasm recursion but call an intrinsic as the last function instead
   string intrinsic_depth_okay = fc::format_string(depth_assert_intrinsic, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth));
   set_code(N(depth), intrinsic_depth_okay.c_str());
   pushit();

   string intrinsic_depth_one_over = fc::format_string(depth_assert_intrinsic, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth+1));
   set_code(N(depth), intrinsic_depth_one_over.c_str());
   BOOST_CHECK_THROW(pushit(), wasm_execution_error);

   //add a float operation in the mix to ensure any injected softfloat call doesn't count against limit
   string wasm_float_depth_okay = fc::format_string(depth_assert_wasm_float, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth));
   set_code(N(depth), wasm_float_depth_okay.c_str());
   pushit();

   string wasm_float_depth_one_over = fc::format_string(depth_assert_wasm_float, fc::mutable_variant_object()
                                              ("MAX_DEPTH", max_call_depth+1));
   set_code(N(depth), wasm_float_depth_one_over.c_str());
   BOOST_CHECK_THROW(pushit(), wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
