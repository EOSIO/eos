#include <eosio/eosio.hpp>

class [[eosio::contract]] kv_table_test : public eosio::contract {
public:
   struct my_struct {
      eosio::name primary_key;
      uint64_t foo;
      std::string bar;

      bool operator==(const my_struct& b) const {
         return primary_key == b.primary_key &&
                foo == b.foo &&
                bar == b.bar;
      }
   };

   struct [[eosio::table]] my_table : eosio::kv::table<my_struct, "kvtable"_n> {
      KV_NAMED_INDEX("primarykey"_n, primary_key)
      KV_NAMED_INDEX("foo"_n, foo)
      KV_NAMED_INDEX("bar"_n, bar)

      my_table(eosio::name contract_name) {
         init(contract_name, primary_key, foo, bar);
      }
   };

   using contract::contract;

   [[eosio::action]]
   void setup() {
      my_table t{"kvtable"_n};
      char base[5] = {'b', 'o', 'b', 'a', 0};
      for (int i = 0; i < 10; ++i) {
         my_struct s;
         s.primary_key = eosio::name{base};
         s.foo = i + 1;
         s.bar = base;
         t.put(s, get_self());

         ++base[3];
      }
      verify();
   }

   [[eosio::action]]
   void verify() {
      my_table t{"kvtable"_n};

      auto val = t.primary_key.get("boba"_n);
      eosio::check(val->primary_key == "boba"_n, "Got the wrong primary value");
      eosio::check(val->foo == 1, "Got the wrong foo value");
      eosio::check(val->bar == "boba", "Got the wrong bar value");

      val = t.primary_key.get("bobj"_n);
      eosio::check(val->primary_key == "bobj"_n, "Got the wrong primary value");
      eosio::check(val->foo == 10, "Got the wrong foo value");
      eosio::check(val->bar == "bobj", "Got the wrong bar value");

      val = t.bar.get("boba");
      eosio::check(val->primary_key == "boba"_n, "Got the wrong primary value");
      eosio::check(val->foo == 1, "Got the wrong foo value");
      eosio::check(val->bar == "boba", "Got the wrong bar value");

      val = t.bar.get("bobj");
      eosio::check(val->primary_key == "bobj"_n, "Got the wrong primary value");
      eosio::check(val->foo == 10, "Got the wrong foo value");
      eosio::check(val->bar == "bobj", "Got the wrong bar value");
     
      val = t.foo.get(1);
      eosio::check(val->primary_key == "boba"_n, "Got the wrong primary value");
      eosio::check(val->foo == 1, "Got the wrong foo value");
      eosio::check(val->bar == "boba", "Got the wrong bar value");

      val = t.foo.get(10);
      eosio::check(val->primary_key == "bobj"_n, "Got the wrong primary value");
      eosio::check(val->foo == 10, "Got the wrong foo value");
      eosio::check(val->bar == "bobj", "Got the wrong bar value");
   }
};
