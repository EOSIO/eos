#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>

// serialization is same as itself
struct kv_config {
   uint32_t version = 0; // must be 0
   uint32_t keysizemax = 128;
   uint32_t valsizemax = 1048576;
   uint32_t itrmax = 1024;
};

extern "C" __attribute__((eosio_wasm_import)) void set_kv_parameters_packed(const void* params, uint32_t size);

#include <eosio/key_value.hpp>
#include <eosio/key_value_singleton.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract]] kv_contract : public eosio::contract {
  public:
  using contract::contract;

   // privileged api
   [[eosio::action]] void setkvparam(name db) {
      kv_config c;
      set_kv_parameters_packed((const char *)&c, sizeof(c));
   }


   struct acc_bal_entry {
      uint64_t      id;
      uint64_t      balance;
      std::string   memo;
      uint64_t by_id() const { return id; }
   };

   struct acc_bal_table_t : eosio::kv_table<acc_bal_entry> {
      KV_NAMED_INDEX("by.id", by_id);

      acc_bal_table_t(eosio::name acc, eosio::name tablename) {
         init(acc, tablename, eosio::kv_ram, by_id);
      }
   };


   [[eosio::action]]
   uint64_t batchinsert(int batch_size);


   // test with txn_gen plugin
   [[eosio::on_notify("txn.test.t::transfer")]]
   uint64_t transfer(const name&   from,
                 const name&    to,
                 const asset&   quantity,
                 const string&  memo) {
      return this->batchinsert(500);
   }
};



uint64_t kv_contract::batchinsert(int batch_size) {
   kv_contract::acc_bal_table_t table(_self, "accbal"_n);
   uint64_t next_id = 0;
   if (table.by_id.begin() != table.by_id.end()) {
      auto itr = table.by_id.end();
      --itr;
      next_id = itr.value().id + 1;
   }

   char randdata[320];
   char *ptr = randdata;
   uint64_t val = next_id;
   for (int i = 0; i < sizeof(randdata); i += 8) {
      val = val * 0x9e3779b97f4a7c13ull;
      *(uint64_t *)((char *)ptr + i ) = val;
   }

   for (int i = 0; i < batch_size; ++i) {

      table.put(kv_contract::acc_bal_entry{.id = next_id, .balance = 1000000, .memo = std::string(randdata, sizeof(randdata)-1)});
      if (next_id >= 10000000) {
         table.erase(kv_contract::acc_bal_entry{.id = next_id - 10000000, .balance = 1000000});
      }
      ++next_id;
   }

  uint64_t final_check = 0;
   if (table.by_id.begin() != table.by_id.end()) {
      auto newitr = table.by_id.end();
      --newitr;
      final_check = newitr.value().id ;
   }
   print("Final check of number of rows in KV table is: ", final_check);




   return next_id;
}

