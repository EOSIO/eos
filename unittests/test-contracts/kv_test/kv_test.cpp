#include <eosio/eosio.hpp>

using namespace eosio;

enum it_stat : int32_t {
   iterator_ok     = 0,
   iterator_erased = -1,
   iterator_end    = -2,
};

#define IMPORT extern "C" __attribute__((eosio_wasm_import))

// clang-format off
IMPORT void     kv_erase(name db, name contract, const char* key, uint32_t key_size);
IMPORT void     kv_set(name db, name contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size);
IMPORT bool     kv_get(name db, name contract, const char* key, uint32_t key_size, uint32_t& value_size);
IMPORT uint32_t kv_get_data(name db, uint32_t offset, char* data, uint32_t data_size);
IMPORT uint32_t kv_it_create(name db, name contract, const char* prefix, uint32_t size);
IMPORT void     kv_it_destroy(uint32_t itr);
IMPORT it_stat  kv_it_status(uint32_t itr);
IMPORT int      kv_it_compare(uint32_t itr_a, uint32_t itr_b);
IMPORT int      kv_it_key_compare(uint32_t itr, const char* key, uint32_t size);
IMPORT it_stat  kv_it_move_to_end(uint32_t itr);
IMPORT it_stat  kv_it_next(uint32_t itr);
IMPORT it_stat  kv_it_prev(uint32_t itr);
IMPORT it_stat  kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size);
IMPORT it_stat  kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
IMPORT it_stat  kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
// clang-format on

struct kv {
   std::vector<char> k;
   std::vector<char> v;
};

class [[eosio::contract("kv_test")]] kvtest : public eosio::contract {
 public:
   using eosio::contract::contract;

   [[eosio::action]] void itlifetime(name db) {
      check(1 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime a");
      check(2 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime b");
      check(3 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime c");
      check(4 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime d");
      check(5 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime e");
      check(6 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime f");
      kv_it_destroy(4);
      kv_it_destroy(2);
      kv_it_destroy(5);
      check(5 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime g");
      check(2 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime h");
      kv_it_destroy(5);
      check(5 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime i");
      check(4 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime j");
      check(7 == kv_it_create(db, ""_n, nullptr, 0), "itlifetime k");
      check(kv_it_status(1) == iterator_end, "itlifetime l");
      check(kv_it_status(2) == iterator_end, "itlifetime m");
      check(kv_it_status(3) == iterator_end, "itlifetime n");
      check(kv_it_status(4) == iterator_end, "itlifetime o");
      check(kv_it_status(5) == iterator_end, "itlifetime p");
      check(kv_it_status(6) == iterator_end, "itlifetime q");
      check(kv_it_status(7) == iterator_end, "itlifetime r");
   }

   [[eosio::action]] void erase(name db, name contract, const std::vector<char>& k) {
      kv_erase(db, contract, k.data(), k.size());
   }

   [[eosio::action]] void set(name db, name contract, const std::vector<char>& k, const std::vector<char>& v) {
      kv_set(db, contract, k.data(), k.size(), v.data(), v.size());
   }

   [[eosio::action]] void get(name db, name contract, const std::vector<char>& k,
                              const std::optional<std::vector<char>>& v) {
      if (v) {
         uint32_t value_size = 0xffff'ffff;
         check(kv_get(db, contract, k.data(), k.size(), value_size), "kv_get found nothing");
         check(value_size == v->size(), "kv_get size mismatch");
         std::vector<char> actual(v->size());
         check(kv_get_data(db, 0, actual.data(), actual.size()) == v->size(), "kv_get_data size mismatch");
         check(actual == *v, "kv_get_data content mismatch");
      } else {
         uint32_t value_size = 0xffff'ffff;
         check(!kv_get(db, contract, k.data(), k.size(), value_size), "kv_get found something");
      }
   }

   [[eosio::action]] void setmany(name db, name contract, const std::vector<kv>& kvs) {
      for (auto& kv : kvs) //
         kv_set(db, contract, kv.k.data(), kv.k.size(), kv.v.data(), kv.v.size());
   }

   [[eosio::action]] void scan(name db, name contract, const std::vector<char>& prefix,
                               const std::optional<std::vector<char>>& lower, const std::vector<kv>& expected) {
      auto    itr = kv_it_create(db, contract, prefix.data(), prefix.size());
      it_stat stat;
      if (lower)
         stat = kv_it_lower_bound(itr, lower->data(), lower->size());
      else
         stat = kv_it_next(itr);
      for (auto& exp : expected) {
         check(stat == iterator_ok, "missing kv pairs");
         check(stat == kv_it_status(itr), "status mismatch (a)");

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

         stat = kv_it_next(itr);
      }
      check(stat == iterator_end, "extra kv pair (g)");
      check(stat == kv_it_status(itr), "status mismatch (g)");

      kv_it_destroy(itr);
   } // scan()

   [[eosio::action]] void scanrev(name db, name contract, const std::vector<char>& prefix,
                                  const std::optional<std::vector<char>>& lower, const std::vector<kv>& expected) {
      auto    itr = kv_it_create(db, contract, prefix.data(), prefix.size());
      it_stat stat;
      if (lower)
         stat = kv_it_lower_bound(itr, lower->data(), lower->size());
      else
         stat = kv_it_prev(itr);
      for (auto& exp : expected) {
         check(stat == iterator_ok, "missing kv pairs");
         check(stat == kv_it_status(itr), "status mismatch (a)");

         // check kv_it_key
         {
            uint32_t k_size = 0xffff'ffff;
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

         stat = kv_it_prev(itr);
      }
      check(stat == iterator_end, "extra kv pair (g)");
      check(stat == kv_it_status(itr), "status mismatch (g)");

      kv_it_destroy(itr);
   } // scanrev()

}; // kvtest
