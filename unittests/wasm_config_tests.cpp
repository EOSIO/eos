#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

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
                               bios_abi_ser.variant_to_binary("setwparams", fc::mutable_variant_object()("cfg", params),
                                                              abi_serializer::create_yield_function( abi_serializer_max_time )));
      trx.actions[0].authorization = vector<permission_level>{{N(eosio),config::active_name}};
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(eosio), "active"), control->get_chain_id());
      push_transaction(trx);
   }
   // Pushes an empty action
   void push_action(account_name account) {
       signed_transaction trx;
       trx.actions.push_back({{{account,config::active_name}}, account, name(), {}});
       set_transaction_headers(trx);
       trx.sign(get_private_key( account, "active" ), control->get_chain_id());
       push_transaction(trx);
   }
   chain::abi_serializer bios_abi_ser;
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

struct old_wasm_tester : tester {
   old_wasm_tester() : tester{setup_policy::old_wasm_parser} {}
};

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


static const char many_funcs_wast[] = R"=====(
(module
  (export "apply" (func 0))
  ${SECTION}
)
)=====";
static const char one_func[] =  "(func (param i64 i64 i64))";

static const char many_types_wast[] = R"=====(
(module
  ${SECTION}
  (export "apply" (func 0))
  (func (type 0))
)
)=====";
static const char one_type[] =  "(type (func (param i64 i64 i64)))";

static const char many_imports_wast[] = R"=====(
(module
  ${SECTION}
  (func (export "apply") (param i64 i64 i64))
)
)=====";
static const char one_import[] =  "(func (import \"env\" \"abort\"))";

static const char many_globals_wast[] = R"=====(
(module
  ${SECTION}
  (func (export "apply") (param i64 i64 i64))
)
)=====";
static const char one_global[] =  "(global i32 (i32.const 0))";

static const char many_exports_wast[] = R"=====(
(module
  ${SECTION}
  (func (export "apply") (param i64 i64 i64))
)
)=====";
static const char one_export[] =  "(export \"fn${N}\" (func 0))";

static const char many_elem_wast[] = R"=====(
(module
  (table 0 anyfunc)
  ${SECTION}
  (func (export "apply") (param i64 i64 i64))
)
)=====";
static const char one_elem[] =  "(elem (i32.const 0))";

static const char many_data_wast[] = R"=====(
(module
  (memory 1)
  ${SECTION}
  (func (export "apply") (param i64 i64 i64))
)
)=====";
static const char one_data[] =  "(data (i32.const 0))";

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_section_elements,
                       data::make({1024, 8192, 16384}) * data::make({0, 1}) *
                       (data::make({many_funcs_wast, many_types_wast, many_imports_wast, many_globals_wast, many_elem_wast, many_data_wast}) ^
                        data::make({one_func       , one_type       , one_import,        one_global       , one_elem      , one_data})),
                       n_elements, oversize, wast, one_element) {
   produce_blocks(2);
   create_accounts({N(section)});
   produce_block();

   std::string buf;
   for(int i = 0; i < n_elements + oversize; ++i) {
      buf += one_element;
   }
   std::string code = fc::format_string(wast, fc::mutable_variant_object("SECTION", buf));

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_section_elements = n_elements;
   set_wasm_params(params);

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(section), code.c_str()), wasm_exception);
   } else {
      set_code(N(section), code.c_str());
      push_action(N(section));
      --params.max_section_elements;
      set_wasm_params(params);
      produce_block();
      push_action(N(section));
      produce_block();
      set_code(N(section), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(section), code.c_str()), wasm_exception);
   }
}

// export has to be formatted slightly differently because export names
// must be unique and apply must be one of the exports.
BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_section_elements_export,
                       data::make({1024, 8192, 16384}) * data::make({0, 1}),
                       n_elements, oversize) {
   produce_blocks(2);
   create_accounts({N(section)});
   produce_block();

   std::string buf;
   for(int i = 0; i < n_elements + oversize - 1; ++i) {
      buf += "(export \"fn$";
      buf += std::to_string(i);
      buf += "\" (func 0))";
   }
   std::string code = fc::format_string(many_exports_wast, fc::mutable_variant_object("SECTION", buf));

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_section_elements = n_elements;
   set_wasm_params(params);

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(section), code.c_str()), wasm_exception);
   } else {
      set_code(N(section), code.c_str());
      push_action(N(section));
      --params.max_section_elements;
      set_wasm_params(params);
      produce_block();
      push_action(N(section));
      produce_block();
      set_code(N(section), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(section), code.c_str()), wasm_exception);
   }
}

static const char max_linear_memory_wast[] = R"=====(
(module
  (import "env" "eosio_assert" (func $$eosio_assert (param i32 i32)))
  (memory 4)
  (data (i32.const ${OFFSET}) "\11\22\33\44")
  (func (export "apply") (param i64 i64 i64)
    (call $$eosio_assert (i32.eq (i32.load (i32.const ${OFFSET})) (i32.const 0x44332211)) (i32.const 0))
  )
)
)=====";

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_linear_memory_init,
                       data::make({32768, 65536, 86513, 131072}) * data::make({0, 1}),
                       n_init, oversize) {
   produce_blocks(2);
   create_accounts({N(initdata)});
   produce_block();

   std::string code = fc::format_string(max_linear_memory_wast, fc::mutable_variant_object("OFFSET", n_init + oversize - 4));

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_linear_memory_init = n_init;
   set_wasm_params(params);

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(initdata), code.c_str()), wasm_exception);
   } else {
      set_code(N(initdata), code.c_str());
      push_action(N(initdata));
      --params.max_linear_memory_init;
      set_wasm_params(params);
      produce_block();
      push_action(N(initdata));
      produce_block();
      set_code(N(initdata), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(initdata), code.c_str()), wasm_exception);
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

BOOST_DATA_TEST_CASE_F(wasm_config_tester, max_nested_structures,
                       data::make({512, 1024, 2048}) * data::make({0, 1}),
                       n_nesting, oversize) {
   produce_block();
   create_accounts( { N(nested) } );
   produce_block();

   std::string code = [&]{
      std::ostringstream ss;
      ss << "(module ";
      ss << " (func (export \"apply\") (param i64 i64 i64) ";
      for(int i = 0; i < n_nesting + oversize - 1; ++i)
         ss << "(block ";
      for(int i = 0; i < n_nesting + oversize - 1; ++i)
         ss << ")";
      ss << " )";
      ss << ")";
      return ss.str();
   }();

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_nested_structures = n_nesting;
   set_wasm_params(params);

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(nested), code.c_str()), wasm_exception);
   } else {
      set_code(N(nested), code.c_str());
      push_action(N(nested));
      --params.max_nested_structures;
      set_wasm_params(params);
      produce_block();
      push_action(N(nested));
      produce_block();
      set_code(N(nested), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(nested), code.c_str()), wasm_exception);
   }
}

static const char max_symbol_func_wast[] = R"=====(
(module
  (func (export "apply") (param i64 i64 i64))
  (func (export "${NAME}"))
)
)=====";

static const char max_symbol_global_wast[] = R"=====(
(module
  (global (export "${NAME}") i32 (i32.const 0))
  (func (export "apply") (param i64 i64 i64))
)
)=====";

static const char max_symbol_memory_wast[] = R"=====(
(module
  (memory (export "${NAME}") 0)
  (func (export "apply") (param i64 i64 i64))
)
)=====";

static const char max_symbol_table_wast[] = R"=====(
(module
  (table (export "${NAME}") 0 anyfunc)
  (func (export "apply") (param i64 i64 i64))
)
)=====";

BOOST_DATA_TEST_CASE_F( wasm_config_tester, max_symbol_bytes_export, data::make({4096, 8192, 16384}) * data::make({0, 1}) *
                        data::make({max_symbol_func_wast, max_symbol_global_wast, max_symbol_memory_wast, max_symbol_table_wast}),
                        n_symbol, oversize, wast ) {
   produce_blocks(2);

   create_accounts({N(bigname)});

   std::string name(n_symbol + oversize, 'x');
   std::string code = fc::format_string(wast, fc::mutable_variant_object("NAME", name));

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_symbol_bytes = n_symbol;
   set_wasm_params(params);

   if(oversize) {
      BOOST_CHECK_THROW(set_code(N(bigname), code.c_str()), wasm_exception);
   } else {
      set_code(N(bigname), code.c_str());
      push_action(N(bigname));
      --params.max_symbol_bytes;
      set_wasm_params(params);
      produce_block();
      push_action(N(bigname));
      produce_block();
      set_code(N(bigname), vector<uint8_t>{}); // clear existing code
      BOOST_CHECK_THROW(set_code(N(bigname), code.c_str()), wasm_exception);
   }
}

static const char max_symbol_import_wast[] = R"=====(
(module
  (func (import "env" "db_idx_long_double_find_secondary") (param i64 i64 i64 i32 i32) (result i32))
  (func (export "apply") (param i64 i64 i64))
)
)=====";

BOOST_FIXTURE_TEST_CASE( max_symbol_bytes_import, wasm_config_tester ) {
   produce_blocks(2);
   create_accounts({N(bigname)});

   constexpr int n_symbol = 33;

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_symbol_bytes = n_symbol;
   set_wasm_params(params);

   set_code(N(bigname), max_symbol_import_wast);
   push_action(N(bigname));
   --params.max_symbol_bytes;
   set_wasm_params(params);
   produce_block();
   push_action(N(bigname));
   produce_block();
   set_code(N(bigname), vector<uint8_t>{}); // clear existing code
   BOOST_CHECK_THROW(set_code(N(bigname), max_symbol_import_wast), wasm_exception);
}

static const std::vector<uint8_t> small_contract_wasm{
   0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00,
   0x01, 0x07, 0x01, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00,
   0x03, 0x02, 0x01, 0x00,
   0x07, 0x09, 0x01, 0x05, 'a', 'p', 'p', 'l', 'y', 0x00, 0x00,
   0x0a, 0xE3, 0x01, 0x01, 0xE0, 0x01,
   0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0b
};

BOOST_FIXTURE_TEST_CASE( max_module_bytes, wasm_config_tester ) {
   produce_blocks(2);
   create_accounts({N(bigmodule)});

   const int n_module = small_contract_wasm.size();

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_module_bytes = n_module;
   set_wasm_params(params);

   set_code(N(bigmodule), small_contract_wasm);
   push_action(N(bigmodule));
   --params.max_module_bytes;
   set_wasm_params(params);
   produce_block();
   push_action(N(bigmodule));
   produce_block();
   set_code(N(bigmodule), vector<uint8_t>{}); // clear existing code
   BOOST_CHECK_THROW(set_code(N(bigmodule), small_contract_wasm), wasm_exception);
}

BOOST_FIXTURE_TEST_CASE( max_code_bytes, wasm_config_tester ) {
   produce_blocks(2);
   create_accounts({N(bigcode)});

   constexpr int n_code = 224;

   auto params = genesis_state::default_initial_wasm_configuration;
   params.max_code_bytes = n_code;
   set_wasm_params(params);

   set_code(N(bigcode), small_contract_wasm);
   push_action(N(bigcode));
   --params.max_code_bytes;
   set_wasm_params(params);
   produce_block();
   push_action(N(bigcode));
   produce_block();
   set_code(N(bigcode), vector<uint8_t>{}); // clear existing code
   BOOST_CHECK_THROW(set_code(N(bigcode), small_contract_wasm), wasm_exception);
}

static const char access_biggest_memory_wast[] = R"=====(
(module
  (memory 0)
  (func (export "apply") (param i64 i64 i64)
    (drop (grow_memory (i32.wrap/i64 (get_local 2))))
    (i32.store (i32.mul (i32.wrap/i64 (get_local 2)) (i32.const 65536)) (i32.const 0))
  )
)
)=====";

BOOST_FIXTURE_TEST_CASE( max_pages, wasm_config_tester ) try {
   produce_blocks(2);

   create_accounts( { N(bigmem), N(accessmem) } );
   set_code(N(accessmem), access_biggest_memory_wast);
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

      // verify that page accessibility cannot leak across wasm executions
      auto checkaccess = [&](uint64_t pagenum) {
         action act;
         act.account = N(accessmem);
         act.name = name(pagenum);
         act.authorization = vector<permission_level>{{N(accessmem),config::active_name}};
         signed_transaction trx;
         trx.actions.push_back(act);

         set_transaction_headers(trx);
         trx.sign(get_private_key( N(accessmem), "active" ), control->get_chain_id());
         BOOST_CHECK_THROW(push_transaction(trx), eosio::chain::wasm_exception);
      };

      pushit(1);
      checkaccess(max_pages - 1);
      produce_blocks(1);

      // Increase memory limit
      ++params.max_pages;
      set_wasm_params(params);
      produce_block();
      pushit(2);
      checkaccess(max_pages);

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

// This contract is one of the smallest that can be used to reset
// the wasm parameters.  It should be impossible to set parameters
// that would prevent it from being set on the eosio account and executing.
static const char min_set_parameters_wast[] = R"======(
(module
  (import "env" "set_wasm_parameters_packed" (func $set_wasm_parameters_packed (param i32 i32)))
  (import "env" "read_action_data" (func $read_action_data (param i32 i32) (result i32)))
  (memory 1)
  (func (export "apply") (param i64 i64 i64)
     (br_if 0 (i32.eqz (i32.eqz (i32.wrap/i64 (get_local 2)))))
     (drop (call $read_action_data (i32.const 4) (i32.const 44)))
     (call $set_wasm_parameters_packed (i32.const 0) (i32.const 48))
  )
)
)======";

BOOST_FIXTURE_TEST_CASE(reset_chain_tests, wasm_config_tester) {
   produce_block();

   wasm_config min_params = {
      .max_mutable_global_bytes = 0,
      .max_table_elements       = 0,
      .max_section_elements     = 4,
      .max_linear_memory_init   = 0,
      .max_func_local_bytes     = 8,
      .max_nested_structures    = 1,
      .max_symbol_bytes         = 32,
      .max_module_bytes         = 256,
      .max_code_bytes           = 32,
      .max_pages                = 1,
      .max_call_depth           = 2
   };

   auto check_minimal = [&](auto& member) {
      if (member > 0) {
         --member;
         BOOST_CHECK_THROW(set_wasm_params(min_params), fc::exception);
         ++member;
      }
   };
   check_minimal(min_params.max_mutable_global_bytes);
   check_minimal(min_params.max_table_elements);
   check_minimal(min_params.max_section_elements);
   check_minimal(min_params.max_func_local_bytes);
   check_minimal(min_params.max_linear_memory_init);
   check_minimal(min_params.max_nested_structures);
   check_minimal(min_params.max_symbol_bytes);
   check_minimal(min_params.max_module_bytes);
   check_minimal(min_params.max_code_bytes);
   check_minimal(min_params.max_pages);
   check_minimal(min_params.max_call_depth);

   set_wasm_params(min_params);
   produce_block();

   // Reset parameters and system contract
   {
      signed_transaction trx;
      auto make_setcode = [](const std::vector<uint8_t>& code) {
         return setcode{ N(eosio), 0, 0, bytes(code.begin(), code.end()) };
      };
      trx.actions.push_back({ { { N(eosio), config::active_name} }, make_setcode(wast_to_wasm(min_set_parameters_wast)) });
      trx.actions.push_back({ { { N(eosio), config::active_name} }, N(eosio), N(), fc::raw::pack(genesis_state::default_initial_wasm_configuration) });
      trx.actions.push_back({ { { N(eosio), config::active_name} }, make_setcode(contracts::eosio_bios_wasm()) });
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(eosio), "active"), control->get_chain_id());
      push_transaction(trx);
   }
   produce_block();
   // Make sure that a normal contract works
   set_wasm_params(genesis_state::default_initial_wasm_configuration);
   produce_block();
}

BOOST_AUTO_TEST_SUITE_END()
