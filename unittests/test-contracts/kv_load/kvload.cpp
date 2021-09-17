//  This contract is based on cdt-develop-box.
//  It is used to simulate KV load. It subscribes to txn_test_gen_plugin's
//  transfer action. Every transfer in txn_test_gen_plugin calls transfer 
//  in this contract to perform multiple KV operations. 

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/key_value.hpp>
#include <eosio/key_value_singleton.hpp>

struct kv_config {
   uint32_t version = 0;
   uint32_t keysizemax = 128;
   uint32_t valsizemax = 1048576;
   uint32_t itrmax = 1024;
};

extern "C" __attribute__((eosio_wasm_import)) void set_kv_parameters_packed(const void*        params, uint32_t size);

using namespace eosio;
using namespace std;

class [[eosio::contract]] kvload: public eosio::contract {
  public:
  using contract::contract;

   [[eosio::action]] void setkvparam(name db) {
      kv_config c;
      set_kv_parameters_packed((const char *)&c, sizeof(c));
   }

   struct row {
      uint64_t      key;
      std::string   value;
      uint64_t by_key() const { return key; }
   };

   struct table_t : eosio::kv_table<row> {
      KV_NAMED_INDEX("by.id", by_key);

      table_t(eosio::name acc, eosio::name tablename) {
         init(acc, tablename, eosio::kv_ram, by_key);
      }
   };

   // Called from txn_gen plugin per transaction
   [[eosio::on_notify("txn.test.t::transfer")]]
   uint64_t transfer(const name&   from,
                 const name&    to,
                 const asset&   quantity,
                 const string&  memo)
   {
      kvload::table_t table(_self, "kvload"_n);
      uint64_t next_key = 0;
      if (table.by_key.begin() != table.by_key.end()) {
         auto itr = table.by_key.end();
         --itr;
         next_key = itr.value().key + 1;
      }

      char data[200];
      for (int i = 0; i < sizeof(data); ++i) {
         data[i] = 'a';
      }

      // Amplify number of KV operations by 20 times per transaction
      for (int i = 0; i < 20; ++i) {
         table.put(kvload::row{.key = next_key, .value = std::string(data, sizeof(data)-1)});
         if (next_key >= 10000000) {
            table.erase(kvload::row{.key = next_key - 10000000});
         }
         ++next_key;
      }

      return next_key;
   }
};
