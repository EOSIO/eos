#pragma once

#include <b1/rodeos/callbacks/kv.hpp>
#include <b1/rodeos/constants.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/to_key.hpp>

namespace b1::rodeos {

using contract_index_double = eosio::ship_protocol::contract_index_double;
using contract_index128     = eosio::ship_protocol::contract_index128;
using contract_index256     = eosio::ship_protocol::contract_index256;
using contract_index64      = eosio::ship_protocol::contract_index64;
using contract_row          = eosio::ship_protocol::contract_row;
using contract_table        = eosio::ship_protocol::contract_table;

struct chaindb_iterator_base {
   int32_t                                 table_index = {};
   uint64_t                                primary     = {};
   int32_t                                 next        = -1;
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

   template <typename Key>
   struct table_index_key {
      int32_t table_index = {};
      Key     key         = {};

      std::tuple<int32_t, Key> order() const { return { table_index, key }; }
      friend bool operator<(const table_index_key& a, const table_index_key& b) { return a.order() < b.order(); }
   };

   std::vector<iterator>                        iterators;
   std::vector<iterator>                        end_iterators;
   std::map<table_index_key<uint64_t>, int32_t> primary_key_to_iterator_index;

   iterator_cache_base(chain_kv::view& view) : view{ view } {}

   Derived& derived() { return static_cast<Derived&>(*this); }

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

   template <typename Ship_index_type, typename T, typename M, typename F1, typename F2>
   int32_t get_iterator_impl(int32_t table_index, const std::optional<T>& key, chain_kv::view::iterator&& view_it,
                             const M& key_to_iterator_index, bool view_it_is_secondary, F1 match, F2 get_key) {
      iterator* it;
      int32_t   result;

      bool found = false;
      if (view_it.is_end()) {
         it     = &end_iterators[table_index];
         result = index_to_end_iterator(table_index);
         found  = true;
      }
      if (!found && key) {
         auto map_it = key_to_iterator_index.find({ table_index, *key });
         if (map_it != key_to_iterator_index.end()) {
            it     = &iterators[map_it->second];
            result = map_it->second;
            found  = true;
         }
      }
      if (!found) {
         eosio::input_stream                    stream;
         std::shared_ptr<const chain_kv::bytes> v;
         if (view_it_is_secondary) {
            v      = view.get(state_account.value, view_it.get_kv()->value);
            stream = { v->data(), v->size() };
         } else {
            stream = { view_it.get_kv()->value.data(), view_it.get_kv()->value.size() };
         }
         auto record = std::get<0>(eosio::from_bin<Ship_index_type>(stream));
         if (!match(record))
            return index_to_end_iterator(table_index);
         auto map_it = key_to_iterator_index.find({ table_index, get_key(record) });
         if (map_it != key_to_iterator_index.end()) {
            it     = &iterators[map_it->second];
            result = map_it->second;
         } else {
            if (iterators.size() > std::numeric_limits<int32_t>::max())
               throw std::runtime_error("too many iterators");
            std::tie(result, it) = derived().create_iterator(table_index, record);
         }
      }
      if (!it->view_it)
         it->view_it = std::move(view_it);
      return result;
   } // get_iterator_impl

   template <typename T>
   std::pair<size_t, iterator*> create_iterator(int32_t table_index, T& record) {
      auto pos = iterators.size();
      iterators.emplace_back();
      auto* it                                                           = &iterators.back();
      it->table_index                                                    = table_index;
      it->primary                                                        = record.primary_key;
      primary_key_to_iterator_index[{ table_index, record.primary_key }] = pos;
      return { pos, it };
   }

   template <typename Ship_index_type, typename F1, typename F2>
   int32_t next_impl(int itr, uint64_t& primary, bool view_it_is_secondary, F1 create_view_it, F2 get_iterator) {
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
      if (!view_it)
         view_it = create_view_it(it, true);
      ++*view_it;
      if (view_it->is_end()) {
         it.next = index_to_end_iterator(it.table_index);
         return it.next;
      } else {
         eosio::input_stream                    stream;
         std::shared_ptr<const chain_kv::bytes> v;
         if (view_it_is_secondary) {
            v      = view.get(state_account.value, view_it->get_kv()->value);
            stream = { v->data(), v->size() };
         } else {
            stream = { view_it->get_kv()->value.data(), view_it->get_kv()->value.size() };
         }
         auto record = std::get<0>(eosio::from_bin<Ship_index_type>(stream));
         primary     = record.primary_key;
         it.next     = get_iterator(it.table_index, std::move(record), std::move(*view_it));
         return it.next;
      }
   } // next_impl

   template <typename Ship_index_type, typename F1, typename F2>
   int32_t prev_impl(int itr, uint64_t& primary, bool view_it_is_secondary, F1 create_view_it, F2 get_iterator) {
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
      if (!view_it)
         view_it = create_view_it(*it, itr >= 0);
      --*view_it;
      if (view_it->is_end()) {
         it->prev = -1;
         return *it->prev;
      } else {
         eosio::input_stream                    stream;
         std::shared_ptr<const chain_kv::bytes> v;
         if (view_it_is_secondary) {
            v      = view.get(state_account.value, view_it->get_kv()->value);
            stream = { v->data(), v->size() };
         } else {
            stream = { view_it->get_kv()->value.data(), view_it->get_kv()->value.size() };
         }
         auto record = std::get<0>(eosio::from_bin<Ship_index_type>(stream));
         primary     = record.primary_key;
         it->prev    = get_iterator(it->table_index, record, std::move(*view_it));
         return *it->prev;
      }
   } // prev_impl

   // Precondition: std::numeric_limits<int32_t>::min() < ei < -1
   // Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
   size_t end_iterator_to_index(int32_t ei) const { return (-ei - 2); }
   // Precondition: indx < tables.size() <= std::numeric_limits<int32_t>::max()
   int32_t index_to_end_iterator(size_t indx) const { return -(indx + 2); }

 public:
   int32_t end(uint64_t code, uint64_t scope, uint64_t table) {
      return index_to_end_iterator(get_table_index({ code, table, scope }));
   }
};

struct chaindb_primary_iterator : chaindb_iterator_base {
   std::vector<char> value;
};

class iterator_cache : public iterator_cache_base<iterator_cache, chaindb_primary_iterator> {
 public:
   using base = iterator_cache_base<iterator_cache, chaindb_primary_iterator>;

   iterator_cache(chain_kv::view& view) : iterator_cache_base{ view } {}

   int32_t get_iterator(int32_t table_index, uint64_t key, chain_kv::view::iterator&& view_it,
                        bool require_match_primary = false) {
      return get_iterator_impl<contract_row>(
            table_index, std::optional{ key }, std::move(view_it), primary_key_to_iterator_index, false,
            [&](auto& record) { return !require_match_primary || record.primary_key == key; },
            [&](auto& record) { return record.primary_key; });
   }

   template <typename T>
   std::pair<size_t, iterator*> create_iterator(int32_t table_index, T& record) {
      auto [pos, it] = base::create_iterator(table_index, record);
      it->value.insert(it->value.end(), record.value.pos, record.value.end);
      return { pos, it };
   }

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

   auto create_view_it(const iterator& it, bool exists) {
      const auto&              table_key = tables[it.table_index];
      chain_kv::view::iterator view_it{ view, state_account.value,
                                        chain_kv::to_slice(eosio::convert_to_key(std::make_tuple(
                                              (uint8_t)0x01, eosio::name{ "contract.row" }, eosio::name{ "primary" },
                                              table_key.code, table_key.table, table_key.scope))) };
      if (exists)
         view_it.lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                                   eosio::name{ "primary" }, table_key.code,
                                                                   table_key.table, table_key.scope, it.primary)));
      return view_it;
   }

   int db_next_i64(int itr, uint64_t& primary) {
      return next_impl<contract_row>(
            itr, primary, false,
            [this](auto&&... args) { return create_view_it(std::forward<decltype(args)>(args)...); },
            [this](int32_t table_index, auto&& record, chain_kv::view::iterator&& view_it) {
               return get_iterator(table_index, record.primary_key, std::move(view_it));
            });
   }

   int db_previous_i64(int itr, uint64_t& primary) {
      return prev_impl<contract_row>(
            itr, primary, false,
            [this](auto&&... args) { return create_view_it(std::forward<decltype(args)>(args)...); },
            [this](int32_t table_index, auto&& record, chain_kv::view::iterator&& view_it) {
               return get_iterator(table_index, record.primary_key, std::move(view_it));
            });
   }

   int32_t lower_bound(uint64_t code, uint64_t scope, uint64_t table, uint64_t key, bool require_exact) {
      int32_t table_index = get_table_index({ code, table, scope });
      if (table_index < 0)
         return -1;
      auto map_it = primary_key_to_iterator_index.find({ table_index, key });
      if (map_it != primary_key_to_iterator_index.end())
         return map_it->second;
      chain_kv::view::iterator it{ view, state_account.value,
                                   chain_kv::to_slice(eosio::convert_to_key(
                                         std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                         eosio::name{ "primary" }, code, table, scope))) };
      it.lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, eosio::name{ "contract.row" },
                                                           eosio::name{ "primary" }, code, table, scope, key)));
      return get_iterator(table_index, key, std::move(it), require_exact);
   }

   int32_t upper_bound(uint64_t code, uint64_t scope, uint64_t table, uint64_t key) {
      if (key + 1 == 0)
         return end(code, scope, table);
      return lower_bound(code, scope, table, key + 1, false);
   }
}; // iterator_cache

template <typename Ship_index_type>
struct chaindb_secondary_iterator : chaindb_iterator_base {
   using secondary_type = std::decay_t<decltype(std::get<0>(std::declval<Ship_index_type>()).secondary_key)>;

   secondary_type secondary = {};
};

template <typename Ship_index_type>
class secondary_iterator_cache : public iterator_cache_base<secondary_iterator_cache<Ship_index_type>,
                                                            chaindb_secondary_iterator<Ship_index_type>> {
 public:
   using base =
         iterator_cache_base<secondary_iterator_cache<Ship_index_type>, chaindb_secondary_iterator<Ship_index_type>>;
   using secondary_type = std::decay_t<decltype(std::get<0>(std::declval<Ship_index_type>()).secondary_key)>;
   using typename base::iterator;

   eosio::name table_name;

   struct whole_key {
      secondary_type secondary = {};
      uint64_t       primary   = {};

      auto        order() const { return std::tie(secondary, primary); }
      friend bool operator<(const whole_key& a, const whole_key& b) { return a.order() < b.order(); }
   };

   std::map<typename base::template table_index_key<whole_key>, int32_t> secondary_to_iterator_index;

   secondary_iterator_cache(chain_kv::view& view, eosio::name table_name) : base{ view }, table_name{ table_name } {}

   template <typename T>
   std::pair<size_t, iterator*> create_iterator(int32_t table_index, T& record) {
      auto [pos, it] = base::create_iterator(table_index, record);
      it->secondary  = record.secondary_key;
      secondary_to_iterator_index[{ table_index, { record.secondary_key, record.primary_key } }] = pos;
      return { pos, it };
   }

   int32_t get_primary_iterator(int32_t table_index, uint64_t key, chain_kv::view::iterator&& view_it,
                                bool require_match_primary = false) {
      return base::template get_iterator_impl<Ship_index_type>(
            table_index, std::optional{ key }, std::move(view_it), base::primary_key_to_iterator_index, false,
            [&](auto& record) { return !require_match_primary || record.primary_key == key; },
            [&](auto& record) { return record.primary_key; });
   }

   int32_t get_secondary_iterator(int32_t table_index, chain_kv::view::iterator&& view_it,
                                  const secondary_type& search_secondary, secondary_type& found_secondary,
                                  std::optional<uint64_t> search_primary, uint64_t& found_primary,
                                  bool require_match_secondary) {
      auto itr = base::template get_iterator_impl<Ship_index_type>(
            table_index,
            search_primary ? std::optional{ whole_key{ search_secondary, *search_primary } } : std::nullopt,
            std::move(view_it), secondary_to_iterator_index, true,
            [&](auto& record) { return !require_match_secondary || record.secondary_key == search_secondary; },
            [&](auto& record) {
               return whole_key{ record.secondary_key, record.primary_key };
            });
      if (itr < 0)
         return itr;
      found_secondary = base::iterators[itr].secondary;
      found_primary   = base::iterators[itr].primary;
      return itr;
   }

   auto create_view_it(const iterator& it, bool exists) {
      const auto&              table_key = base::tables[it.table_index];
      chain_kv::view::iterator view_it{ base::view, state_account.value,
                                        chain_kv::to_slice(eosio::convert_to_key(
                                              std::make_tuple((uint8_t)0x01, table_name, eosio::name{ "secondary" },
                                                              table_key.code, table_key.table, table_key.scope))) };
      if (exists)
         view_it.lower_bound(eosio::convert_to_key(
               std::make_tuple((uint8_t)0x01, table_name, eosio::name{ "secondary" }, table_key.code, table_key.table,
                               table_key.scope, it.secondary, it.primary)));
      return view_it;
   }

   int next(int itr, uint64_t& primary) {
      return base::template next_impl<Ship_index_type>(
            itr, primary, true,
            [this](auto&&... args) { return create_view_it(std::forward<decltype(args)>(args)...); },
            [this](int32_t table_index, auto&& record, chain_kv::view::iterator&& view_it) {
               return get_secondary_iterator(table_index, std::move(view_it), record.secondary_key,
                                             record.secondary_key, record.primary_key, record.primary_key, false);
            });
   }

   int prev(int itr, uint64_t& primary) {
      return base::template prev_impl<Ship_index_type>(
            itr, primary, true,
            [this](auto&&... args) { return create_view_it(std::forward<decltype(args)>(args)...); },
            [this](int32_t table_index, auto&& record, chain_kv::view::iterator&& view_it) {
               return get_secondary_iterator(table_index, std::move(view_it), record.secondary_key,
                                             record.secondary_key, record.primary_key, record.primary_key, false);
            });
   }

   int32_t lower_bound(uint64_t code, uint64_t scope, uint64_t table, secondary_type& secondary, uint64_t& primary,
                       bool require_exact) {
      int32_t table_index = base::get_table_index({ code, table, scope });
      if (table_index < 0)
         return -1;
      auto map_it = secondary_to_iterator_index.lower_bound({ table_index, { secondary, 0 } });
      if (map_it != secondary_to_iterator_index.end() && map_it->first.key.secondary == secondary) {
         primary = map_it->first.key.primary;
         return map_it->second;
      }
      chain_kv::view::iterator it{ base::view, state_account.value,
                                   chain_kv::to_slice(eosio::convert_to_key(std::make_tuple(
                                         (uint8_t)0x01, table_name, eosio::name{ "secondary" }, code, table, scope))) };
      it.lower_bound(eosio::convert_to_key(
            std::make_tuple((uint8_t)0x01, table_name, eosio::name{ "secondary" }, code, table, scope, secondary)));
      return get_secondary_iterator(table_index, std::move(it), secondary, secondary, 0, primary, require_exact);
   }

   int32_t upper_bound(uint64_t code, uint64_t scope, uint64_t table, secondary_type& secondary, uint64_t& primary) {
      int32_t table_index = base::get_table_index({ code, table, scope });
      if (table_index < 0)
         return -1;
      chain_kv::view::iterator it{ base::view, state_account.value,
                                   chain_kv::to_slice(eosio::convert_to_key(std::make_tuple(
                                         (uint8_t)0x01, table_name, eosio::name{ "secondary" }, code, table, scope))) };
      it.lower_bound(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, table_name, eosio::name{ "secondary" }, code,
                                                           table, scope, secondary, ~uint64_t(0), uint8_t(0))));
      return get_secondary_iterator(table_index, std::move(it), secondary, secondary, std::nullopt, primary, false);
   }

   int32_t find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, secondary_type& secondary) {
      int32_t table_index = base::get_table_index({ code, table, scope });
      if (table_index < 0)
         return -1;
      auto map_it = base::primary_key_to_iterator_index.find({ table_index, primary });
      if (map_it != base::primary_key_to_iterator_index.end()) {
         if (map_it->second >= 0)
            secondary = base::iterators[map_it->second].secondary;
         return map_it->second;
      }
      chain_kv::view::iterator it{ base::view, state_account.value,
                                   chain_kv::to_slice(eosio::convert_to_key(std::make_tuple(
                                         (uint8_t)0x01, table_name, eosio::name{ "primary" }, code, table, scope))) };
      it.lower_bound(eosio::convert_to_key(
            std::make_tuple((uint8_t)0x01, table_name, eosio::name{ "primary" }, code, table, scope, primary)));
      return get_primary_iterator(table_index, primary, std::move(it), true);
   }
};

struct chaindb_state {
   std::unique_ptr<rodeos::iterator_cache>                          iterator_cache;
   std::unique_ptr<secondary_iterator_cache<contract_index64>>      idx64;
   std::unique_ptr<secondary_iterator_cache<contract_index128>>     idx128;
   std::unique_ptr<secondary_iterator_cache<contract_index_double>> idx_double;
   std::unique_ptr<secondary_iterator_cache<contract_index256>>     idx256;
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

#define IMPL_SECONDARY(NAME, TYPE, TABLE_NAME)                                                                         \
   auto& get_##NAME() {                                                                                                \
      auto& chaindb_state = derived().get_chaindb_state();                                                             \
      if (!chaindb_state.NAME)                                                                                         \
         chaindb_state.NAME = std::make_unique<decltype(chaindb_state::NAME)::element_type>(                           \
               derived().get_db_view_state().kv_state.view, TABLE_NAME);                                               \
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
      return get_##NAME().next(iterator, *primary);                                                                    \
   }                                                                                                                   \
   int32_t db_##NAME##_previous(int32_t iterator, legacy_ptr<uint64_t> primary) {                                      \
      return get_##NAME().prev(iterator, *primary);                                                                    \
   }                                                                                                                   \
   int32_t db_##NAME##_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,         \
                                    uint64_t primary) {                                                                \
      return get_##NAME().find_primary(code, scope, table, primary, *secondary);                                       \
   }                                                                                                                   \
   int32_t db_##NAME##_find_secondary(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const TYPE> secondary, \
                                      legacy_ptr<uint64_t> primary) {                                                  \
      auto s = *secondary;                                                                                             \
      return get_##NAME().lower_bound(code, scope, table, s, *primary, true);                                          \
   }                                                                                                                   \
   int32_t db_##NAME##_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,           \
                                  legacy_ptr<uint64_t> primary) {                                                      \
      return get_##NAME().lower_bound(code, scope, table, *secondary, *primary, false);                                \
   }                                                                                                                   \
   int32_t db_##NAME##_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<TYPE> secondary,           \
                                  legacy_ptr<uint64_t> primary) {                                                      \
      return get_##NAME().upper_bound(code, scope, table, *secondary, *primary);                                       \
   }                                                                                                                   \
   int32_t db_##NAME##_end(uint64_t code, uint64_t scope, uint64_t table) {                                            \
      return get_##NAME().end(code, scope, table);                                                                     \
   }

   IMPL_SECONDARY(idx64, uint64_t, eosio::name{ "contract.i1" })
   IMPL_SECONDARY(idx128, __uint128_t, eosio::name{ "contract.i2" })
   IMPL_SECONDARY(idx_double, double, eosio::name{ "contract.dbl" })

#undef IMPL_SECONDARY

   auto& get_idx256() {
      auto& chaindb_state = derived().get_chaindb_state();
      if (!chaindb_state.idx256)
         chaindb_state.idx256 = std::make_unique<decltype(chaindb_state::idx256)::element_type>(
               derived().get_db_view_state().kv_state.view, eosio::name{ "contract.cs" });
      return *chaindb_state.idx256;
   }
   template <typename T>
   eosio::checksum256 read_cs(legacy_span<T>& arr_128) {
      if (arr_128.size_bytes() != 32)
         throw std::runtime_error("checksum256 must be 32 bytes");
      __uint128_t a[2];
      memcpy(a, arr_128.data(), 32);
      return { a };
   }
   void write_cs(legacy_span<__uint128_t>& arr_128, const eosio::checksum256& cs) {
      if (arr_128.size_bytes() != 32)
         throw std::runtime_error("checksum256 must be 32 bytes");
      auto a = cs.extract_as_word_array<__uint128_t>();
      memcpy(arr_128.data(), a.data(), 32);
   }
   int32_t db_idx256_store(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,
                           legacy_span<const __uint128_t> secondary) {
      throw std::runtime_error("unimplemented: db_idx256_store");
   }
   void db_idx256_update(int32_t iterator, uint64_t payer, legacy_span<const __uint128_t> secondary) {
      throw std::runtime_error("unimplemented: db_idx256_update");
   }
   void    db_idx256_remove(int32_t iterator) { throw std::runtime_error("unimplemented: db_idx256_remove"); }
   int32_t db_idx256_next(int32_t iterator, legacy_ptr<uint64_t> primary) {
      return get_idx256().next(iterator, *primary);
   }
   int32_t db_idx256_previous(int32_t iterator, legacy_ptr<uint64_t> primary) {
      return get_idx256().prev(iterator, *primary);
   }
   int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, legacy_span<__uint128_t> arr_128,
                                  uint64_t primary) {
      auto cs     = read_cs(arr_128);
      auto result = get_idx256().find_primary(code, scope, table, primary, cs);
      write_cs(arr_128, cs);
      return result;
   }
   int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                    legacy_span<const __uint128_t> secondary, legacy_ptr<uint64_t> primary) {
      auto cs = read_cs(secondary);
      return get_idx256().lower_bound(code, scope, table, cs, *primary, true);
   }
   int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, legacy_span<__uint128_t> secondary,
                                legacy_ptr<uint64_t> primary) {
      auto cs     = read_cs(secondary);
      auto result = get_idx256().lower_bound(code, scope, table, cs, *primary, false);
      write_cs(secondary, cs);
      return result;
   }
   int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, legacy_span<__uint128_t> secondary,
                                legacy_ptr<uint64_t> primary) {
      auto cs     = read_cs(secondary);
      auto result = get_idx256().upper_bound(code, scope, table, cs, *primary);
      write_cs(secondary, cs);
      return result;
   }
   int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) { return get_idx256().end(code, scope, table); }

#define REGISTER_SECONDARY(IDX)                                                                                        \
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

      REGISTER_SECONDARY(idx64)
      REGISTER_SECONDARY(idx128)
      REGISTER_SECONDARY(idx_double)
      REGISTER_SECONDARY(idx256)
   }

#undef REGISTER_SECONDARY
};

} // namespace b1::rodeos
