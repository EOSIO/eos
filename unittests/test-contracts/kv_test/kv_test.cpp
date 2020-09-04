#include <eosio/eosio.hpp>


using namespace eosio;
using namespace eosio::internal_use_do_not_use;

enum it_stat : int32_t {
   iterator_ok     = 0,
   iterator_erased = -1,
   iterator_end    = -2,
};

struct kv {
   std::vector<char> k;
   std::vector<char> v;
};

struct itparam {
   name db;
   uint32_t count;
   bool erase;
};

#define STR_I(x) #x
#define STR(x) STR_I(x)
#define TEST(condition) check(condition, "kv_test.cpp:" STR(__LINE__) ": " #condition) 

class [[eosio::contract("kv_test")]] kvtest : public eosio::contract {
 public:
   using eosio::contract::contract;

   [[eosio::action]] void itlifetime(name db) {
      check(1 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime a");
      check(2 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime b");
      check(3 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime c");
      check(4 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime d");
      check(5 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime e");
      check(6 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime f");
      kv_it_destroy(4);
      kv_it_destroy(2);
      kv_it_destroy(5);
      check(5 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime g");
      check(2 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime h");
      kv_it_destroy(5);
      check(5 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime i");
      check(4 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime j");
      check(7 == kv_it_create(db.value, ""_n.value, nullptr, 0), "itlifetime k");
      check(kv_it_status(1) == iterator_end, "itlifetime l");
      check(kv_it_status(2) == iterator_end, "itlifetime m");
      check(kv_it_status(3) == iterator_end, "itlifetime n");
      check(kv_it_status(4) == iterator_end, "itlifetime o");
      check(kv_it_status(5) == iterator_end, "itlifetime p");
      check(kv_it_status(6) == iterator_end, "itlifetime q");
      check(kv_it_status(7) == iterator_end, "itlifetime r");
   }

   [[eosio::action]] void itlimit(std::vector<itparam> params) {
      bool has_erase = false;
      for(auto& itop : params) {
         if(itop.erase) {
            has_erase = true;
            break;
         }
      }
      if(!has_erase) {
         for(auto itop : params) {
            for(uint32_t i = 0; i < itop.count; ++i) {
               kv_it_create(itop.db.value, ""_n.value, nullptr, 0);
            }
         }
      } else {
         std::map<name, std::vector<int>> iterators;
         for(auto itop : params) {
            auto& iters = iterators[itop.db];
            if(itop.erase) {
               for(uint32_t i = 0; i < itop.count; ++i) {
                  kv_it_destroy(iters.back());
                  iters.pop_back();
               }
            } else {
               for(uint32_t i = 0; i < itop.count; ++i) {
                  iters.push_back(kv_it_create(itop.db.value, ""_n.value, nullptr, 0));
               }
            }
         }
      }
   }

   [[eosio::action]] void erase(name db, name contract, const std::vector<char>& k) {
      kv_erase(db.value, contract.value, k.data(), k.size());
   }

   [[eosio::action]] void set(name db, name contract, const std::vector<char>& k, const std::vector<char>& v, name payer) {
      kv_set(db.value, contract.value, k.data(), k.size(), v.data(), v.size(), payer.value);
   }

   [[eosio::action]] void get(name db, name contract, const std::vector<char>& k,
                              const std::optional<std::vector<char>>& v) {
      if (v) {
         uint32_t value_size = 0xffff'ffff;
         check(kv_get(db.value, contract.value, k.data(), k.size(), value_size), "kv_get found nothing");
         check(value_size == v->size(), "kv_get size mismatch");
         std::vector<char> actual(v->size());
         check(kv_get_data(db.value, 0, actual.data(), actual.size()) == v->size(), "kv_get_data size mismatch");
         check(actual == *v, "kv_get_data content mismatch");
      } else {
         uint32_t value_size = 0xffff'ffff;
         check(!kv_get(db.value, contract.value, k.data(), k.size(), value_size), "kv_get found something");
      }
   }

   [[eosio::action]] void setmany(name db, name contract, const std::vector<kv>& kvs) {
      for (auto& kv : kvs) //
         kv_set(db.value, contract.value, kv.k.data(), kv.k.size(), kv.v.data(), kv.v.size(), contract.value);
   }

   [[eosio::action]] void getdata(name db) {
      const std::vector<char> orig_buf(1024, '\xcc');
      std::vector<char> buf = orig_buf;
      // The buffer starts empty
      TEST(kv_get_data(db.value, 0, buf.data(), 0) == 0);
      TEST(buf == orig_buf); // unchanged
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      TEST(buf == orig_buf); // unchanged
      TEST(kv_get_data(db.value, 0xFFFFFFFFu, buf.data(), 0) == 0);
      TEST(buf == orig_buf); // unchanged
      TEST(kv_get_data(db.value, 0xFFFFFFFFu, buf.data(), 1024) == 0);
      TEST(buf == orig_buf); // unchanged
      // Add a value and load it into the temporary buffer
      kv_set(db.value, get_self().value, "key", 3, "value", 5, get_self().value);
      TEST(kv_get_data(db.value, 0, nullptr, 0) == 0);
      uint32_t value_size = 0xffffffff;
      kv_get(db.value, get_self().value, "key", 3, value_size);
      // Test different offsets
      TEST(kv_get_data(db.value, 0, nullptr, 0) == 5);
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 5);
      TEST(memcmp(buf.data(), "value\xcc\xcc\xcc", 8) == 0);
      buf = orig_buf;
      TEST(kv_get_data(db.value, 0xFFFFFFFFu, buf.data(), 1024) == 5);
      TEST(buf == orig_buf);
      TEST(kv_get_data(db.value, 1, buf.data(), 1024) == 5);
      TEST(memcmp(buf.data(), "alue\xcc\xcc\xcc\xcc", 8) == 0);
      buf = orig_buf;
      TEST(kv_get_data(db.value, 5, buf.data(), 1024) == 5);
      TEST(buf == orig_buf);
      TEST(kv_get_data(db.value, 4, buf.data(), 1024) == 5);
      TEST(memcmp(buf.data(), "e\xcc\xcc\xcc\xcc\xcc\xcc\xcc", 8) == 0);
      buf = orig_buf;
      // kv_get with missing key clears buffer
      kv_get(db.value, get_self().value, "", 0, value_size);
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      // kv_set clears the buffer
      kv_get(db.value, get_self().value, "key", 3, value_size);
      kv_set(db.value, get_self().value, "key2", 4, "", 0, get_self().value); // set another key
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      kv_get(db.value, get_self().value, "key", 3, value_size);
      kv_set(db.value, get_self().value, "key", 3, "value", 5, get_self().value); // same key
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      // kv_erase clears the buffer
      kv_get(db.value, get_self().value, "key", 3, value_size);
      kv_erase(db.value, get_self().value, "", 0); // key does not exist
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      kv_get(db.value, get_self().value, "key", 3, value_size);
      kv_erase(db.value, get_self().value, "key2", 4); // other key
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
      kv_get(db.value, get_self().value, "key", 3, value_size);
      kv_erase(db.value, get_self().value, "key", 3); // this key
      TEST(kv_get_data(db.value, 0, buf.data(), 1024) == 0);
   }

   [[eosio::action]] void scan(name db, name contract, const std::vector<char>& prefix,
                               const std::optional<std::vector<char>>& lower, const std::vector<kv>& expected) {
      auto    itr = kv_it_create(db.value, contract.value, prefix.data(), prefix.size());
      int32_t stat;
      uint32_t key_size, value_size;
      if (lower)
         stat = kv_it_lower_bound(itr, lower->data(), lower->size(), key_size, value_size);
      else
         stat = kv_it_next(itr, key_size, value_size);
      for (auto& exp : expected) {
         check(stat == iterator_ok, "missing kv pairs");
         check(stat == kv_it_status(itr), "status mismatch (a)");
         check(key_size == exp.k.size(), "wrong key_size from kv_it_next");
         check(value_size == exp.v.size(), "wrong value_size from kv_it_next");

         // check kv_it_key
         for (uint32_t offset = 0; offset <= exp.k.size(); ++offset) {
            uint32_t k_size = 0xffff'ffff;
            check(kv_it_key(itr, 0, nullptr, 0, k_size) == iterator_ok && k_size == exp.k.size(),
                  "key has wrong size (a)");

            { // use offset
               std::vector<char> k(k_size - offset);
               k.push_back(42);
               k.push_back(53);
               k_size = 0xffff'ffff;
               check(kv_it_key(itr, offset, k.data(), k.size() - 2, k_size) == iterator_ok && k_size == exp.k.size(),
                     "key has wrong size (b)");
               check(!memcmp(exp.k.data() + offset, k.data(), k.size() - 2), "key has wrong content (b)");
               check(k[k.size() - 2] == 42 && k[k.size() - 1] == 53, "buffer overrun (b)");
            }

            { // offset=0, truncate
               std::vector<char> k(k_size - offset);
               k.push_back(42);
               k.push_back(53);
               k_size = 0xffff'ffff;
               check(kv_it_key(itr, 0, k.data(), k.size() - 2, k_size) == iterator_ok && k_size == exp.k.size(),
                     "key has wrong size (c)");
               check(!memcmp(exp.k.data(), k.data(), k.size() - 2), "key has wrong content (c)");
               check(k[k.size() - 2] == 42 && k[k.size() - 1] == 53, "buffer overrun (c)");
            }
         }

         // check kv_it_value
         for (uint32_t offset = 0; offset <= exp.v.size(); ++offset) {
            uint32_t v_size = 0xffff'ffff;
            check(kv_it_value(itr, 0, nullptr, 0, v_size) == iterator_ok && v_size == exp.v.size(),
                  "value has wrong size (d)");

            { // use offset
               std::vector<char> v(v_size - offset);
               v.push_back(42);
               v.push_back(53);
               v_size = 0xffff'ffff;
               check(kv_it_value(itr, offset, v.data(), v.size() - 2, v_size) == iterator_ok && v_size == exp.v.size(),
                     "value has wrong size (e)");
               check(!memcmp(exp.v.data() + offset, v.data(), v.size() - 2), "value has wrong content (e)");
               check(v[v.size() - 2] == 42 && v[v.size() - 1] == 53, "buffer overrun (e)");
            }

            { // offset=0, truncate
               std::vector<char> v(v_size - offset);
               v.push_back(42);
               v.push_back(53);
               v_size = 0xffff'ffff;
               check(kv_it_value(itr, 0, v.data(), v.size() - 2, v_size) == iterator_ok && v_size == exp.v.size(),
                     "value has wrong size (f)");
               check(!memcmp(exp.v.data(), v.data(), v.size() - 2), "value has wrong content (f)");
               check(v[v.size() - 2] == 42 && v[v.size() - 1] == 53, "buffer overrun (f)");
            }
         }

         stat = kv_it_next(itr, key_size, value_size);
      }
      check(key_size == 0, "expected end to have 0 key size");
      check(value_size == 0, "expected end to have 0 value size");
      check(stat == iterator_end, "extra kv pair (g)");
      check(stat == kv_it_status(itr), "status mismatch (g)");

      kv_it_destroy(itr);
   } // scan()

   [[eosio::action]] void scanrev(name db, name contract, const std::vector<char>& prefix,
                                  const std::optional<std::vector<char>>& lower, const std::vector<kv>& expected) {
      auto    itr = kv_it_create(db.value, contract.value, prefix.data(), prefix.size());
      int32_t stat;
      uint32_t key_size = 0xffff'ffff, value_size = 0xffff'ffff;
      if (lower)
         stat = kv_it_lower_bound(itr, lower->data(), lower->size(), key_size, value_size);
      else
         stat = kv_it_prev(itr, key_size, value_size);
      for (auto& exp : expected) {
         std::string message = "missing kv pairs: ";
         message.append(exp.k.data(), exp.k.size());
         check(stat == iterator_ok, message.c_str());
         check(stat == kv_it_status(itr), "status mismatch (a)");

         // check kv_it_key
         {
            uint32_t k_size = 0xffff'ffff;
            check(key_size == exp.k.size(), "wrong key_size from kv_it_prev");
            check(kv_it_key(itr, 0, nullptr, 0, k_size) == iterator_ok && k_size == exp.k.size(),
                  "key has wrong size (a)");
            std::vector<char> k(k_size);
            k.push_back(42);
            k.push_back(53);
            k_size = 0xffff'ffff;
            check(kv_it_key(itr, 0, k.data(), k.size() - 2, k_size) == iterator_ok && k_size == exp.k.size(),
                  "key has wrong size (b)");
            check(!memcmp(exp.k.data(), k.data(), k.size() - 2), "key has wrong content (b)");
            check(k[k.size() - 2] == 42 && k[k.size() - 1] == 53, "buffer overrun (b)");
         }

         // check kv_it_value
         {
            uint32_t v_size = 0xffff'ffff;
            check(value_size == exp.v.size(), "wrong value_size from kv_it_prev");
            check(kv_it_value(itr, 0, nullptr, 0, v_size) == iterator_ok && v_size == exp.v.size(),
                  "value has wrong size (d)");
            std::vector<char> v(v_size);
            v.push_back(42);
            v.push_back(53);
            v_size = 0xffff'ffff;
            check(kv_it_value(itr, 0, v.data(), v.size() - 2, v_size) == iterator_ok && v_size == exp.v.size(),
                  "value has wrong size (e)");
            check(!memcmp(exp.v.data(), v.data(), v.size() - 2), "value has wrong content (e)");
            check(v[v.size() - 2] == 42 && v[v.size() - 1] == 53, "buffer overrun (e)");
         }

         stat = kv_it_prev(itr, key_size, value_size);
      }
      check(key_size == 0, "expected end to have 0 key size");
      check(value_size == 0, "expected end to have 0 value size");
      check(stat == iterator_end, "extra kv pair (g)");
      check(stat == kv_it_status(itr), "status mismatch (g)");

      kv_it_destroy(itr);
   } // scanrev()

   [[eosio::action]] void itstaterased(name db, name contract, const std::vector<char>& prefix,
                                       const std::vector<char>& k, const std::vector<char>& v,
                                       int test_id, bool insert, bool reinsert) {
      if(insert) kv_set(db.value, contract.value, k.data(), k.size(), v.data(), v.size(), contract.value);
      auto it = kv_it_create(db.value, contract.value, prefix.data(), prefix.size());
      uint32_t key_size, value_size;
      int32_t stat = kv_it_lower_bound(it, k.data(), k.size(), key_size, value_size);
      kv_erase(db.value, contract.value, k.data(), k.size());
      check(kv_it_status(it) == iterator_erased, "iterator should be erased");
      auto check_status = [&](int test_id) {
         switch(test_id) {
            case 0: {
               uint32_t k_size;
               char k_buf[1];
               kv_it_key(it, 0, k_buf, 0, k_size); // abort
               return;
            }
            case 1: {
               uint32_t v_size;
               char v_buf[1];
               kv_it_value(it, 0, v_buf, 0, v_size); // abort
               return;
            }
            case 2: {
               auto it2 = kv_it_create(db.value, contract.value, prefix.data(), prefix.size());
               kv_it_compare(it, it2); // abort
               return;
            }
            case 3: {
               auto it2 = kv_it_create(db.value, contract.value, prefix.data(), prefix.size());
               kv_it_compare(it2, it); // abort
               return;
            }
            case 4: {
               kv_it_compare(it, it); // abort
               return;
            }
            case 5: {
               kv_it_key_compare(it, k.data(), k.size()); // abort
               return;
            }
            case 6: {
               kv_it_next(it, key_size, value_size); // abort
               return;
            }
            case 7: {
               kv_it_prev(it, key_size, value_size); // abort
               return;
            }
            case 8: {
               check(kv_it_move_to_end(it) == iterator_end, "expected end");
               check(kv_it_status(it) == iterator_end, "expected end");
               return;
            }
            case 9: {
               // For this test there must be no other elements in range
               check(kv_it_lower_bound(it, "", 0, key_size, value_size) == iterator_end, "expected end");
               check(kv_it_status(it) == iterator_end, "expected end");
               return;
            }
            case 10: {
               // For this test there must be at least one other element in range
               check(kv_it_lower_bound(it, "", 0, key_size, value_size) == iterator_ok, "expected an element");
               check(kv_it_status(it) == iterator_ok, "expected an element");
               return;
            }
            case 11: {
               kv_it_destroy(it); // We should be able to destroy iterators successfully
               return;
            }
         }
      };
      if(!reinsert) {
         check_status(test_id);
         return;
      }
      kv_set(db.value, contract.value, k.data(), k.size(), v.data(), v.size(), contract.value);
      check_status(test_id);
   } // itstaterased

}; // kvtest
