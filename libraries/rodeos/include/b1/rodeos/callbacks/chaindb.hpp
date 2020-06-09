#pragma once

#include <b1/rodeos/callbacks/kv.hpp>
#include <b1/rodeos/constants.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/to_key.hpp>

namespace b1::rodeos {

using contract_index128 = eosio::ship_protocol::contract_index128;
using contract_index64  = eosio::ship_protocol::contract_index64;
using contract_row      = eosio::ship_protocol::contract_row;
using contract_table    = eosio::ship_protocol::contract_table;

struct chaindb_iterator_base {
   int32_t                                 table_index = {};
   uint64_t                                primary     = {};
   std::vector<char>                       value;
   int32_t                                 next = -1;
   std::optional<int32_t>                  prev;
   std::optional<chain_kv::view::iterator> view_it;
};

template <typename Derived, typename Iterator>
class iterator_cache_base {
 protected:
   using iterator = Iterator;

   chain_kv::view& view;

   struct table_key {
      uint64_t code  = {};
      uint64_t table = {};
      uint64_t scope = {};

      std::tuple<uint64_t, uint64_t, uint64_t> order() const { return { code, table, scope }; }
      friend bool operator<(const table_key& a, const table_key& b) { return a.order() < b.order(); }
   };
   std::vector<table_key>       tables;
   std::map<table_key, int32_t> table_to_index;

   struct row_key {
      int32_t  table_index = {};
      uint64_t key         = {};

      std::tuple<int32_t, uint64_t> order() const { return { table_index, key }; }
      friend bool                   operator<(const row_key& a, const row_key& b) { return a.order() < b.order(); }
   };

   std::vector<iterator>      iterators;
   std::vector<iterator>      end_iterators;
   std::map<row_key, int32_t> key_to_iterator_index;

   iterator_cache_base(chain_kv::view& view) : view{ view } {}

   Derived& derived() { return static_cast<Derived&>(*this); } // !!!

   int32_t get_table_index(table_key key) {
      auto map_it = table_to_index.find(key);
      if (map_it != table_to_index.end())
         return map_it->second;
      if (!view.get(state_account.value, chain_kv::to_slice(eosio::convert_to_key(std::make_tuple(
                                               (uint8_t)0x01, eosio::name{ "contract.tab" }, eosio::name{ "primary" },
                                               key.code, key.table, key.scope)))))
         return -1;
      if (tables.size() != table_to_index.size() || tables.size() != end_iterators.size())
         throw std::runtime_error("internal error: tables.size() mismatch");
      auto result = tables.size();
      if (result > std::numeric_limits<int32_t>::max())
         throw std::runtime_error("too many open tables");
      tables.push_back(key);
      table_to_index[key] = result;
      end_iterators.push_back({});
      auto& end_it       = end_iterators.back();
      end_it.table_index = result;
      return result;
   }

   int32_t get_iterator(row_key rk, chain_kv::view::iterator&& view_it, bool require_exact = false) {
      iterator* it;
      int32_t   result;
      if (view_it.is_end()) {
         it     = &end_iterators[rk.table_index];
         result = index_to_end_iterator(rk.table_index);
      } else {
         auto map_it = key_to_iterator_index.find(rk);
         if (map_it != key_to_iterator_index.end()) {
            it     = &iterators[map_it->second];
            result = map_it->second;
         } else {
            eosio::input_stream stream{ view_it.get_kv()->value.data(), view_it.get_kv()->value.size() };
            auto                row = std::get<0>(eosio::from_bin<contract_row>(stream));
            if (require_exact && row.primary_key != rk.key)
               return index_to_end_iterator(rk.table_index);
            map_it = key_to_iterator_index.find({ rk.table_index, row.primary_key });
            if (map_it != key_to_iterator_index.end()) {
               it     = &iterators[map_it->second];
               result = map_it->second;
            } else {
               if (iterators.size() > std::numeric_limits<int32_t>::max())
                  throw std::runtime_error("too many iterators");
               result = iterators.size();
               iterators.emplace_back();
               it              = &iterators.back();
               it->table_index = rk.table_index;
               it->primary     = row.primary_key;
               it->value.insert(it->value.end(), row.value.pos, row.value.end);
               key_to_iterator_index[{ rk.table_index, row.primary_key }] = result;
            }
         }
      }
      if (!it->view_it)
         it->view_it = std::move(view_it);
      return result;
   }

   // Precondition: std::numeric_limits<int32_t>::min() < ei < -1
   // Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
   size_t end_iterator_to_index(int32_t ei) const { return (-ei - 2); }
   // Precondition: indx < tables.size() <= std::numeric_limits<int32_t>::max()
   int32_t index_to_end_iterator(size_t indx) const { return -(indx + 2); }
};

struct chaindb_primary_iterator : chaindb_iterator_base {};

class iterator_cache : public iterator_cache_base<iterator_cache, chaindb_primary_iterator> {
 public:
   iterator_cache(chain_kv::view& view) : iterator_cache_base{ view } {}

   size_t db_get_i64(int itr, char* buffer, uint32_t buffer_size) {
      if (itr == -1)
         throw std::runtime_error("dereference invalid iterator");
      if (itr < 0)
         throw std::runtime_error("dereference end iterator");
      if (size_t(itr) >= iterators.size())
         throw std::runtime_error("dereference non-existing iterator");
      auto& it = iterators[itr];
      return legacy_copy_to_wasm(buffer, buffer_size, it.value.data(), it.value.size());
   }

   int db_next_i64(int itr, uint64_t& primary) {
      if (itr == -1)
         throw std::runtime_error("increment invalid iterator");
      if (itr < 0)
         return -1;
      if (size_t(itr) >= iterators.size())
         throw std::runtime_error("increment non-existing iterator");
      auto& it = iterators[itr];
      if (it.next >= 0) {
         primary = iterators[it.next].primary;
         return it.next;
      } else if (it.next < -1) {
         return it.next;
      }
      std::optional<chain_kv::view::iterator> view_it = std::move(it.view_it);
      it.view_it.reset();
      if (!view_it) {
         const auto& table_key = tables[it.table_index];
         view_it =
               chain_kv::view::iterator{ view, state_account.value,
                                         chain_kv::to_slice(eosio::convert_to_key(std::make_tuple( //
                                               (uint8_t)0x01, eosio::name{ "contract.row" }, eosio::name{ "primary" },
                                               table_key.code, table_key.table, table_key.scope))) };
         view_it->lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                                    eosio::name{ "primary" }, table_key.code,
                                                                    table_key.table, table_key.scope, it.primary)));
      }
      ++*view_it;
      if (view_it->is_end()) {
         it.next = index_to_end_iterator(it.table_index);
         return it.next;
      } else {
         eosio::input_stream stream{ view_it->get_kv()->value.data(), view_it->get_kv()->value.size() };
         auto                row = std::get<0>(eosio::from_bin<contract_row>(stream));
         primary                 = row.primary_key;
         it.next                 = get_iterator({ it.table_index, primary }, std::move(*view_it));
         return it.next;
      }
   } // db_next_i64

   int db_previous_i64(int itr, uint64_t& primary) {
      if (itr == -1)
         throw std::runtime_error("decrement invalid iterator");
      iterator* it = nullptr;
      if (std::numeric_limits<int32_t>::min() < itr && itr < -1) {
         size_t table_index = end_iterator_to_index(itr);
         if (table_index >= end_iterators.size())
            throw std::runtime_error("decrement non-existing iterator");
         it = &end_iterators[table_index];
      } else {
         if (size_t(itr) >= iterators.size())
            throw std::runtime_error("decrement non-existing iterator");
         it = &iterators[itr];
      }
      if (it->prev) {
         if (*it->prev >= 0)
            primary = iterators[*it->prev].primary;
         return *it->prev;
      }
      std::optional<chain_kv::view::iterator> view_it = std::move(it->view_it);
      it->view_it.reset();
      if (!view_it) {
         const auto& table_key = tables[it->table_index];
         view_it =
               chain_kv::view::iterator{ view, state_account.value,
                                         chain_kv::to_slice(eosio::convert_to_key(std::make_tuple( //
                                               (uint8_t)0x01, eosio::name{ "contract.row" }, eosio::name{ "primary" },
                                               table_key.code, table_key.table, table_key.scope))) };
         if (itr >= 0)
            view_it->lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                                       eosio::name{ "primary" }, table_key.code,
                                                                       table_key.table, table_key.scope, it->primary)));
      }
      --*view_it;
      if (view_it->is_end()) {
         it->prev = -1;
         return *it->prev;
      } else {
         eosio::input_stream stream{ view_it->get_kv()->value.data(), view_it->get_kv()->value.size() };
         auto                row = std::get<0>(eosio::from_bin<contract_row>(stream));
         primary                 = row.primary_key;
         it->prev                = get_iterator({ it->table_index, primary }, std::move(*view_it));
         return *it->prev;
      }
   } // db_previous_i64

   int32_t lower_bound(uint64_t code, uint64_t scope, uint64_t table, uint64_t key, bool require_exact) {
      int32_t table_index = get_table_index({ code, table, scope });
      if (table_index < 0)
         return -1;
      row_key rk{ table_index, key };
      auto    map_it = key_to_iterator_index.find(rk);
      if (map_it != key_to_iterator_index.end())
         return map_it->second;
      chain_kv::view::iterator it{ view, state_account.value,
                                   chain_kv::to_slice(eosio::convert_to_key(
                                         std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                         eosio::name{ "primary" }, code, table, scope))) };
      it.lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                           eosio::name{ "primary" }, code, table, scope, key)));
      return get_iterator(rk, std::move(it), require_exact);
   }

   int32_t upper_bound(uint64_t code, uint64_t scope, uint64_t table, uint64_t key) {
      if (key + 1 == 0)
         return end(code, scope, table);
      return lower_bound(code, scope, table, key + 1, false);
   }

   int32_t end(uint64_t code, uint64_t scope, uint64_t table) {
      return index_to_end_iterator(get_table_index({ code, table, scope }));
   }
}; // iterator_cache

template <typename Ship_index_type>
struct chaindb_secondary_iterator : chaindb_iterator_base {};

template <typename Ship_index_type>
class secondary_iterator_cache : public iterator_cache_base<secondary_iterator_cache<Ship_index_type>,
                                                            chaindb_secondary_iterator<Ship_index_type>> {
 public:
   using base =
         iterator_cache_base<secondary_iterator_cache<Ship_index_type>, chaindb_secondary_iterator<Ship_index_type>>;
   using secondary_type = std::decay_t<decltype(std::get<0>(std::declval<Ship_index_type>()).secondary_key)>;

   eosio::name table_name;

   secondary_iterator_cache(chain_kv::view& view, eosio::name table_name) : base{ view }, table_name{ table_name } {}
};

struct chaindb_state {
   std::unique_ptr<rodeos::iterator_cache>                      iterator_cache;
   std::unique_ptr<secondary_iterator_cache<contract_index64>>  idx64;
   std::unique_ptr<secondary_iterator_cache<contract_index128>> idx128;
};

template <typename Derived>
struct chaindb_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   iterator_cache& get_iterator_cache() {
      auto& chaindb_state = derived().get_chaindb_state();
      if (!chaindb_state.iterator_cache)
         chaindb_state.iterator_cache = std::make_unique<iterator_cache>(derived().get_db_view_state().kv_state.view);
      return *chaindb_state.iterator_cache;
   }

   int32_t db_store_i64(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_span<const char> buffer) {
      throw std::runtime_error("unimplemented: db_store_i64");
   }

   void db_update_i64(int32_t itr, uint64_t payer, legacy_span<const char> buffer) {
      throw std::runtime_error("unimplemented: db_update_i64");
   }

   void db_remove_i64(int itr) { throw std::runtime_error("unimplemented: db_remove_i64"); }

   int32_t db_get_i64(int32_t itr, legacy_span<char> buffer) {
      return get_iterator_cache().db_get_i64(itr, buffer.data(), buffer.size());
   }

   int32_t db_next_i64(int32_t itr, legacy_ptr<uint64_t> primary) {
      return get_iterator_cache().db_next_i64(itr, *primary);
   }

   int32_t db_previous_i64(int32_t itr, legacy_ptr<uint64_t> primary) {
      return get_iterator_cache().db_previous_i64(itr, *primary);
   }

   int db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return get_iterator_cache().lower_bound(code, scope, table, id, true);
   }

   int db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return get_iterator_cache().lower_bound(code, scope, table, id, false);
   }

   int db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return get_iterator_cache().upper_bound(code, scope, table, id);
   }

   int db_end_i64(uint64_t code, uint64_t scope, uint64_t table) {
      return get_iterator_cache().end(code, scope, table);
   }

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(NAME, TYPE)                                                                  \
   auto& get_##NAME() {                                                                                                \
      auto& chaindb_state = derived().get_chaindb_state();                                                             \
      if (!chaindb_state.NAME)                                                                                         \
         chaindb_state.NAME =                                                                                          \
               std::make_unique<decltype(*chaindb_state.NAME)>(derived().get_db_view_state().kv_state.view);           \
      return *chaindb_state.NAME;                                                                                      \
   }                                                                                                                   \
   int32_t db_##NAME##_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,                              \
                             legacy_ptr<const TYPE> secondary) {                                                       \
      throw std::runtime_error("unimplemented: db_" #NAME "_store");                                                   \
   }                                                                                                                   \
   void db_##NAME##_update(int32_t iterator, uint64_t payer, legacy_ptr<const TYPE> secondary) {                       \
      throw std::runtime_error("unimplemented: db_" #NAME "_update");                                                  \
   }                                                                                                                   \
   void    db_##NAME##_remove(int32_t iterator) { throw std::runtime_error("unimplemented: db_" #NAME "_remove"); }    \
   int32_t db_##NAME##_next(int32_t iterator, legacy_ptr<uint64_t> primary) {                                          \
      throw std::runtime_error("unimplemented: db_" #NAME "_next");                                                    \
   }                                                                                                                   \
   int32_t db_##NAME##_previous(int32_t iterator, legacy_ptr<uint64_t> primary) {                                      \
      throw std::runtime_error("unimplemented: db_" #NAME "_previous");                                                \
   }                                                                                                                   \
   int32_t db_##NAME##_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,         \
                                    uint64_t primary) {                                                                \
      throw std::runtime_error("unimplemented: db_" #NAME "_find_primary");                                            \
   }                                                                                                                   \
   int32_t db_##NAME##_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const TYPE> secondary, \
                                      legacy_ptr<uint64_t> primary) {                                                  \
      throw std::runtime_error("unimplemented: db_" #NAME "_find_secondary");                                          \
   }                                                                                                                   \
   int32_t db_##NAME##_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,           \
                                  legacy_ptr<uint64_t> primary) {                                                      \
      throw std::runtime_error("unimplemented: db_" #NAME "_lowerbound");                                              \
   }                                                                                                                   \
   int32_t db_##NAME##_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,           \
                                  legacy_ptr<uint64_t> primary) {                                                      \
      throw std::runtime_error("unimplemented: db_" #NAME "_upperbound");                                              \
   }                                                                                                                   \
   int32_t db_##NAME##_end(uint64_t code, uint64_t scope, uint64_t table) {                                            \
      throw std::runtime_error("unimplemented: db_" #NAME "_end");                                                     \
   }

   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64, uint64_t)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128, __uint128_t)

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX)                                                                         \
   Rft::template add<&Derived::db_##IDX##_store>("env", "db_" #IDX "_store");                                          \
   Rft::template add<&Derived::db_##IDX##_remove>("env", "db_" #IDX "_remove");                                        \
   Rft::template add<&Derived::db_##IDX##_update>("env", "db_" #IDX "_update");                                        \
   Rft::template add<&Derived::db_##IDX##_find_primary>("env", "db_" #IDX "_find_primary");                            \
   Rft::template add<&Derived::db_##IDX##_find_secondary>("env", "db_" #IDX "_find_secondary");                        \
   Rft::template add<&Derived::db_##IDX##_lowerbound>("env", "db_" #IDX "_lowerbound");                                \
   Rft::template add<&Derived::db_##IDX##_upperbound>("env", "db_" #IDX "_upperbound");                                \
   Rft::template add<&Derived::db_##IDX##_end>("env", "db_" #IDX "_end");                                              \
   Rft::template add<&Derived::db_##IDX##_next>("env", "db_" #IDX "_next");                                            \
   Rft::template add<&Derived::db_##IDX##_previous>("env", "db_" #IDX "_previous");

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      Rft::template add<&Derived::db_store_i64>("env", "db_store_i64");
      Rft::template add<&Derived::db_update_i64>("env", "db_update_i64");
      Rft::template add<&Derived::db_remove_i64>("env", "db_remove_i64");
      Rft::template add<&Derived::db_get_i64>("env", "db_get_i64");
      Rft::template add<&Derived::db_next_i64>("env", "db_next_i64");
      Rft::template add<&Derived::db_previous_i64>("env", "db_previous_i64");
      Rft::template add<&Derived::db_find_i64>("env", "db_find_i64");
      Rft::template add<&Derived::db_lowerbound_i64>("env", "db_lowerbound_i64");
      Rft::template add<&Derived::db_upperbound_i64>("env", "db_upperbound_i64");
      Rft::template add<&Derived::db_end_i64>("env", "db_end_i64");

      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
   }

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE
};

} // namespace b1::rodeos
