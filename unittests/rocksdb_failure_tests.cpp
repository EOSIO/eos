#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>
#include <chain_kv/chain_kv.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <contracts.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
namespace bdata = boost::unit_test::data;

using mvo = fc::mutable_variant_object;

namespace {

static const char kv_snapshot_bios[] = R"=====(
(module
  (func $set_resource_limit (import "env" "set_resource_limit") (param i64 i64 i64))
  (func $kv_set_parameters_packed (import "env" "set_kv_parameters_packed") (param i64 i32 i32))
  (memory 1)
  (func (export "apply") (param i64 i64 i64)
    (call $kv_set_parameters_packed (i64.const 6138663586874765568) (i32.const 0) (i32.const 16))
    (call $kv_set_parameters_packed (i64.const 6138663586881971200) (i32.const 0) (i32.const 16))
    (call $set_resource_limit (get_local 2) (i64.const 5454140623722381312) (i64.const -1))
  )
  (data (i32.const 4) "\00\04\00\00")
  (data (i32.const 8) "\00\00\10\00")
  (data (i32.const 12) "\80\00\00\00")
)
)=====";

static const char basic_rocksdb_contract[] = R"=====(
(module
  (import "env" "kv_set" (func $kv_set (param i64 i64 i32 i32 i32 i32) (result i64)))
  (import "env" "kv_erase" (func $kv_erase (param i64 i64 i32 i32) (result i64)))
  (memory 1)
  (func (export "apply") (param i64 i64 i64)
    (if (i64.eq (get_local 2) (i64.const 14029275789212516352))
      (then (drop (call $kv_set (i64.const 6138663586874765568) (get_local 0) (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 1))))
      (else (drop (call $kv_erase (i64.const 6138663586874765568) (get_local 0) (i32.const 0) (i32.const 0))))
    )
  )
)
)=====";

class rocksdb_tester : public validating_tester {
public:
   rocksdb_tester() {
      produce_blocks(2);
      create_accounts({ N(rocksdb), N(setup) });
      set_code(N(rocksdb), basic_rocksdb_contract);
      set_code(N(setup), kv_snapshot_bios);
      push_action(N(eosio), N(setpriv), N(eosio), mutable_variant_object()("account", "setup")("is_priv", 1));
      action act;
      act.account = N(setup);
      act.name    = N(rocksdb);
      (void)push_action(std::move(act), N(setup).to_uint64_t());
   }
   void use_rocksdb() {
      eosio::chain::use_rocksdb_for_disk(const_cast<chainbase::database&>(control->db()));
   }
   void set() {
      action act;
      act.account = N(rocksdb);
      act.name    = N(set);
      (void)base_tester::push_action(std::move(act), N(rocksdb).to_uint64_t());
   }
   void erase() {
      action act;
      act.account = N(rocksdb);
      act.name    = N(erase);
      (void)base_tester::push_action(std::move(act), N(rocksdb).to_uint64_t());
   }
   void set_and_erase() {
      try {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(rocksdb), config::active_name}}, N(rocksdb), N(set), bytes{});
         trx.actions.emplace_back(vector<permission_level>{{N(rocksdb), config::active_name}}, N(rocksdb), N(erase), bytes{});
         set_transaction_headers(trx);
         trx.sign(get_private_key(N(rocksdb), "active"), control->get_chain_id());
         push_transaction(trx);
      } catch(...) {}
   }
};
}

BOOST_AUTO_TEST_CASE(test_rocksdb_failure) {
   for(int i = 0; ; ++i) {
      BOOST_TEST_CONTEXT("counter: " << i)
      {
         rocksdb_tester t;
         t.skip_validate = true;
         int& fail_counter = t.control->get_rocksdb_fail_counter();
         fail_counter = i;
         try {
            t.use_rocksdb();
            t.set();
            BOOST_TEST_PASSPOINT();
            t.erase();
            BOOST_TEST_PASSPOINT();
            t.set_and_erase();
            BOOST_TEST_PASSPOINT();
            t.produce_block();
            BOOST_TEST_PASSPOINT();
         } catch(...) {
            // If we bailed out of start_block, the previous block may not have been
            // pushed to the validating node.
            if(t.control->head_block_num() != t.validating_node->head_block_num()) {
               BOOST_TEST(t.control->head_block_num() == t.validating_node->head_block_num() + 1);
               t.validate_push_block(t.control->fetch_block_by_id(t.control->head_block_id()));
            }
         }
         if(fail_counter != 0) {
            break;
         }
         // Make sure that we can successfully resume
         t.control->abort_block();
         fail_counter = -1; // Allow successful close
         t.close();
         t.open();
         t.set();
         t.erase();
         t.produce_block();
         t.check_validate();
      }
   }
}
