#include <eosio/eosio.hpp>
#include <eosio/table.hpp>
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

//#include "eosio/key_value.hpp"
//#include "eosio/key_value_singleton.hpp"

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

   struct [[eosio::table]] numobj {
      uint64_t        key;
      uint64_t        init_id;
      uint64_t        next_id;
      uint64_t        data;

      uint64_t primary_key() const { return key; }
   };

   struct acc_bal_entry {
      uint64_t      id;
      uint64_t      id2;
      uint64_t      id3;
      uint64_t      id4;
      uint64_t      id5;
      uint64_t      id6;
      uint64_t      id7;
      uint64_t      id8;
      uint64_t      id9;
      uint64_t      id10;
      uint64_t      id11;
      uint64_t      id12;
      uint64_t      id13;
      uint64_t      id14;
      uint64_t      id15;
      uint64_t      id16;
      uint64_t      id17;
      uint64_t      id18;
      uint64_t      id19;
      uint64_t      id20;
      uint64_t by_id() const { return id; }
      bool operator==(const acc_bal_entry& b) const {
         return id == b.id;
      }
   };

   struct [[eosio::table]] acc_bal_table_t : eosio::kv::table<acc_bal_entry, "accbal"_n> {
      KV_NAMED_INDEX("id", id);

      acc_bal_table_t(eosio::name acc) {
         init(acc, id);
      }
   };


   [[eosio::action]]
   uint64_t batchinsert(int batch_size);

   [[eosio::action]]
   uint64_t batchsetup(int batch_size);

   [[eosio::action]]
   void oneline(uint64_t id, uint64_t data) {
      acc_bal_table_t table("accbal"_n);
      oneline(table, id, data);
   }

   void oneline(kv_contract::acc_bal_table_t& table, uint64_t id, uint64_t data) {
      acc_bal_entry entry;
      entry.id = id;
   // if any value changes, the whole line is a diff
      entry.id2 = data;
      entry.id3 = data;
      entry.id4 = data;
      entry.id5 = data;
      entry.id6 = data;
      entry.id7 = data;
      entry.id8 = data;
      entry.id9 = data;
      entry.id10 = data;
      entry.id11 = data;
      entry.id12 = data;
      entry.id13 = data;
      entry.id14 = data;
      entry.id15 = data;
      entry.id16 = data;
      entry.id17 = data;
      entry.id18 = data;
      entry.id19 = data;
      entry.id20 = data;
      table.put(entry, get_self());
   }

   // test with txn_gen plugin
   [[eosio::on_notify("txn.test.t::transfer")]]
   uint64_t transfer(const name&   from,
                 const name&    to,
                 const asset&   quantity,
                 const string&  memo) {
      return this->batchinsert(500);
   }

    typedef eosio::multi_index< "numobjs"_n, numobj > numobjs;
};

kv_contract::numobjs::const_iterator get_numobjs(kv_contract::numobjs& table, name self) {
   const uint64_t the_only_key = 0;
   auto itr = table.find(the_only_key);
   if(itr == table.end()) {
      eosio::print("get_numobjs create only entry\n");
      table.emplace(self, [&]( auto& obj ) {
         obj.key = the_only_key;
	 obj.init_id = 0;
	 obj.next_id = 0;
	 obj.data = 0;
      });
      itr = table.find(the_only_key); 
   }
   else {
      eosio::print("get_numobjs returning existing entry init_id: ", itr->init_id, ", next_id: ", itr->next_id, ", data: ", itr->data, "\n");
   }
   return itr;
}

void set_last_id(kv_contract::numobjs& table, name self, const kv_contract::numobj& ref, uint64_t last_id) {
   table.modify(ref, self,  [&](auto& obj) {
      obj.next_id = last_id;
      const uint64_t max = 0xffff'ffff'ffff'ffff;
      obj.data = (ref.data < max) ? ref.data + 1 : 0;
      eosio::print("get_numobjs set_last_id init_id: ", obj.init_id, ", next_id: ", obj.next_id, ", data: ", obj.data, "\n");
   });
}

void set_init_id(kv_contract::numobjs& table, name self, const kv_contract::numobj& ref, uint64_t last_id) {
   table.modify(ref, self,  [&](auto& obj) {
      obj.init_id = last_id;
      eosio::print("get_numobjs set_init_id init_id: ", obj.init_id, ", next_id: ", obj.next_id, ", data: ", obj.data, "\n");
   });
}

uint64_t kv_contract::batchinsert(int batch_size) {
   eosio::print("batchinsert\n");
   acc_bal_table_t table("accbal"_n);
   numobjs numobjs_table( _self, _self.value );
   uint64_t last_id = 0;
   if (table.id.begin() != table.id.end()) {
      auto itr = table.id.end();
      --itr;
      last_id = itr.value().id + 1;
      eosio::print("last_id: ", last_id, "/n");
   }

   uint64_t next_id = 0;
   uint64_t data = 0;
   auto itr = get_numobjs(numobjs_table, _self);
   next_id = itr->next_id;
   data = itr->data;
   eosio::print("next_id: ", next_id, ", data: ", data, "/n");

   uint64_t amount_to_inc = 1;
   auto inc_id = [last_id,&amount_to_inc](uint64_t& id) {
      // don't write past the region of data we initialized
      if (last_id - amount_to_inc < id) {
         const int64_t delta = last_id - id;
         id = amount_to_inc - delta;
      }
      else {
         id += amount_to_inc;
      }
      amount_to_inc <<= 1;
      if (amount_to_inc > 0xffff)
         amount_to_inc = 1;
   };

   for (int i = 0; i < batch_size; ++i, inc_id(next_id)) {
      oneline(table, next_id, data);
   }

   set_last_id(numobjs_table, _self, *itr, next_id);

   return next_id;
};

uint64_t kv_contract::batchsetup(int batch_size) {
   acc_bal_table_t table("accbal"_n);
   numobjs numobjs_table( _self, _self.value );

   auto itr = get_numobjs(numobjs_table, _self);
   uint64_t init_id = itr->init_id;
   eosio::print("init_id: ", init_id, "/n");

   acc_bal_entry entry;
   auto next = 0xffff;
   // if any value changes, the whole line is a diff
   entry.id2 = ++next;
   entry.id3 = ++next;
   entry.id4 = ++next;
   entry.id5 = ++next;
   entry.id6 = ++next;
   entry.id7 = ++next;
   entry.id8 = ++next;
   entry.id9 = ++next;
   entry.id10 = ++next;
   entry.id11 = ++next;
   entry.id12 = ++next;
   entry.id13 = ++next;
   entry.id14 = ++next;
   entry.id15 = ++next;
   entry.id16 = ++next;
   entry.id17 = ++next;
   entry.id18 = ++next;
   entry.id19 = ++next;
   entry.id20 = ++next;
   for (int i = 0; i < batch_size; ++i, ++init_id) {
      entry.id = init_id;
      table.put(entry, get_self());
   }

   set_init_id(numobjs_table, _self, *itr, init_id);

   return init_id;
}
