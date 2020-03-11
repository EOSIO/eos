#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

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
static const char basic_rocksdb_contract[] = R"=====(
(module
  (func (export "apply") (param i64 i64 i64)
    (import "env" "kv_set" (func (param i64 i64 i32 i32 i32 i32)))
    (import "env" "kv_erase" (func (param i64 i64 i32 i32)))
    (if (i64.eq (get_local 2) (i64.const 14029275789212516352))
      (then (call $kv_set (i64.const 6138663586874765568) (get_local 0) (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 1)))
      (else (call $kv_erase (i64.const 6138663586874765568) (get_local 0) (i32.const 0)))
    )
  )
)
)=====";

class rocksdb_tester : public tester {
public:
   rocksdb_tester() {
      produce_blocks(2);
      create_accounts({ N(rocksdb) });
      set_code(N(rocksdb), basic_rocksdb_contract);
   }
   void use_rocksdb() {
      eosio::chain::use_rocksdb_for_disk(const_cast<chainbase::database&>(control->db()));
   }
   void set() {
      action act;
      act.account = N(rocksdb);
      act.name    = N(set);
      base_tester::push_action(std::move(act), N(rocksdb).to_uint64_t());
   }
   void erase() {
      action act;
      act.account = N(rocksdb);
      act.name    = N(erase);
      base_tester::push_action(std::move(act), N(rocksdb).to_uint64_t());
   }
};
}
  
BOOST_AUTO_TEST_CASE(test_rocksdb_failure) {
   for(int i = 0; ; ++i) {
      chain_kv::fail_counter = i;
      tester t;
      try {
         eosio::chain::use_rocksdb_for_disk(const_cast<chainbase::database&>(t.control->db()));
         t.set();
         t.erase();
      } catch(...) {}
      if(chain_kv::fail_counter != 0) {
         break;
      }
      //
      chain_kv::fail_counter = -1;
      t.close();
      t.open();
      t.set();
      t.erase();
   }
}
