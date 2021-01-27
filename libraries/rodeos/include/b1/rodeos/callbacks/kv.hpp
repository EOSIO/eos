#pragma once

#include <b1/chain_kv/chain_kv.hpp>
#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/check.hpp>
#include <eosio/name.hpp>
#include <rocksdb/db.h>
#include <rocksdb/table.h>

namespace b1::rodeos {

inline constexpr eosio::name state_db_id{ "eosio.state" };

enum class kv_it_stat {
   iterator_ok     = 0,  // Iterator is positioned at a key-value pair
   iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
   iterator_end    = -2, // Iterator is out-of-bounds
};

struct kv_iterator_rocksdb {
   uint32_t&                num_iterators;
   chain_kv::view&          view;
   uint64_t                 contract;
   chain_kv::view::iterator kv_it;

   kv_iterator_rocksdb(uint32_t& num_iterators, chain_kv::view& view, uint64_t contract, const char* prefix,
                       uint32_t size)
       : num_iterators(num_iterators), view{ view }, contract{ contract }, kv_it{ view, contract, { prefix, size } } {
      ++num_iterators;
   }

   ~kv_iterator_rocksdb() { --num_iterators; }

   bool is_kv_chainbase_context_iterator() const { return false; }
   bool is_kv_rocksdb_context_iterator() const { return true; }

   kv_it_stat kv_it_status() {
      if (kv_it.is_end())
         return kv_it_stat::iterator_end;
      else if (kv_it.is_erased())
         return kv_it_stat::iterator_erased;
      else
         return kv_it_stat::iterator_ok;
   }

   void fill_found(uint32_t* found_key_size, uint32_t* found_value_size) {
      auto kv = kv_it.get_kv();
      if (kv) {
         if (found_key_size)
            *found_key_size = kv->key.size();
         if (found_value_size)
            *found_value_size = kv->value.size();
      } else {
         if (found_key_size)
            *found_key_size = 0;
         if (found_value_size)
            *found_value_size = 0;
      }
   }

   int32_t kv_it_compare(const kv_iterator_rocksdb& rhs) {
      eosio::check(rhs.is_kv_rocksdb_context_iterator(), "Incompatible key-value iterators");
      auto& r = static_cast<const kv_iterator_rocksdb&>(rhs);
      eosio::check(&view == &r.view && contract == r.contract, "Incompatible key-value iterators");
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");
      eosio::check(!r.kv_it.is_erased(), "Iterator to erased element");
      return compare(kv_it, r.kv_it);
   }

   int32_t kv_it_key_compare(const char* key, uint32_t size) {
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");
      return chain_kv::compare_key(kv_it.get_kv(), chain_kv::key_value{ { key, size }, {} });
   }

   kv_it_stat kv_it_move_to_end() {
      kv_it.move_to_end();
      return kv_it_stat::iterator_end;
   }

   kv_it_stat kv_it_next(uint32_t* found_key_size = nullptr, uint32_t* found_value_size = nullptr) {
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");
      ++kv_it;
      fill_found(found_key_size, found_value_size);
      return kv_it_status();
   }

   kv_it_stat kv_it_prev(uint32_t* found_key_size = nullptr, uint32_t* found_value_size = nullptr) {
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");
      --kv_it;
      fill_found(found_key_size, found_value_size);
      return kv_it_status();
   }

   kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size = nullptr,
                                uint32_t* found_value_size = nullptr) {
      kv_it.lower_bound(key, size);
      fill_found(found_key_size, found_value_size);
      return kv_it_status();
   }

   kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");

      std::optional<chain_kv::key_value> kv;
      kv = kv_it.get_kv();

      if (kv) {
         if (offset < kv->key.size())
            memcpy(dest, kv->key.data() + offset, std::min((size_t)size, kv->key.size() - offset));
         actual_size = kv->key.size();
         return kv_it_stat::iterator_ok;
      } else {
         actual_size = 0;
         return kv_it_stat::iterator_end;
      }
   }

   kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      eosio::check(!kv_it.is_erased(), "Iterator to erased element");

      std::optional<chain_kv::key_value> kv;
      kv = kv_it.get_kv();

      if (kv) {
         if (offset < kv->value.size())
            memcpy(dest, kv->value.data() + offset, std::min((size_t)size, kv->value.size() - offset));
         actual_size = kv->value.size();
         return kv_it_stat::iterator_ok;
      } else {
         actual_size = 0;
         return kv_it_stat::iterator_end;
      }
   }
}; // kv_iterator_rocksdb

struct kv_database_config {
   std::uint32_t max_key_size   = 1024;
   std::uint32_t max_value_size = 256 * 1024; // Large enough to hold most contracts
   std::uint32_t max_iterators  = 1024;
};

struct kv_context_rocksdb {
   chain_kv::database&                      database;
   chain_kv::write_session&                 write_session;
   std::vector<char>                        contract_kv_prefix;
   eosio::name                              database_id;
   chain_kv::view                           view;
   bool                                     enable_write          = false;
   bool                                     bypass_receiver_check = false;
   eosio::name                              receiver;
   const kv_database_config&                limits;
   uint32_t                                 num_iterators = 0;
   std::shared_ptr<const std::vector<char>> temp_data_buffer;

   kv_context_rocksdb(chain_kv::database& database, chain_kv::write_session& write_session,
                      std::vector<char> contract_kv_prefix, eosio::name database_id, eosio::name receiver,
                      const kv_database_config& limits)
       : database{ database }, write_session{ write_session }, contract_kv_prefix{ std::move(contract_kv_prefix) },
         database_id{ database_id }, view{ write_session, make_prefix() }, receiver{ receiver }, limits{ limits } {}

   std::vector<char> make_prefix() {
      std::vector<char> prefix = contract_kv_prefix;
      chain_kv::append_key(prefix, database_id.value);
      return prefix;
   }

   int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) {
      eosio::check(enable_write && (bypass_receiver_check || eosio::name{ contract } == receiver),
                   "Can not write to this key");
      temp_data_buffer = nullptr;
      view.erase(contract, { key, key_size });
      return 0;
   }

   int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size, uint64_t payer) {
      eosio::check(enable_write && (bypass_receiver_check || eosio::name{ contract } == receiver),
                   "Can not write to this key");
      eosio::check(key_size <= limits.max_key_size, "Key too large");
      eosio::check(value_size <= limits.max_value_size, "Value too large");
      temp_data_buffer = nullptr;
      view.set(contract, { key, key_size }, { value, value_size });
      return 0;
   }

   bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) {
      temp_data_buffer = view.get(contract, { key, key_size });
      if (temp_data_buffer) {
         value_size = temp_data_buffer->size();
         return true;
      } else {
         value_size = 0;
         return false;
      }
   }

   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) {
      const char* temp      = nullptr;
      uint32_t    temp_size = 0;
      if (temp_data_buffer) {
         temp      = temp_data_buffer->data();
         temp_size = temp_data_buffer->size();
      }
      if (offset < temp_size)
         memcpy(data, temp + offset, std::min(data_size, temp_size - offset));
      return temp_size;
   }

   std::unique_ptr<kv_iterator_rocksdb> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) {
      eosio::check(num_iterators < limits.max_iterators, "Too many iterators");
      return std::make_unique<kv_iterator_rocksdb>(num_iterators, view, contract, prefix, size);
   }
}; // kv_context_rocksdb

struct db_view_state {
   eosio::name                                       receiver;
   chain_kv::database&                               database;
   const kv_database_config                          limits;
   const kv_database_config                          kv_state_limits{ 1024, std::numeric_limits<uint32_t>::max() };
   kv_context_rocksdb                                kv_state;
   std::vector<std::unique_ptr<kv_iterator_rocksdb>> kv_iterators;
   std::vector<size_t>                               kv_destroyed_iterators;

   db_view_state(eosio::name receiver, chain_kv::database& database, chain_kv::write_session& write_session,
                 const std::vector<char>& contract_kv_prefix)
       : receiver{ receiver }, database{ database }, //
         kv_state{ database, write_session, contract_kv_prefix, state_db_id, receiver, kv_state_limits },
         kv_iterators(1) {}

   void reset() {
      eosio::check(kv_iterators.size() == kv_destroyed_iterators.size() + 1, "iterators are still alive");
      kv_iterators.resize(1);
      kv_destroyed_iterators.clear();
   }
};

template <typename Derived>
struct db_callbacks {
   Derived& derived() { return static_cast<Derived&>(*this); }

   int64_t kv_erase(uint64_t contract, eosio::vm::span<const char> key) {
      return kv_get_db().kv_erase(contract, key.data(), key.size());
   }

   int64_t kv_set(uint64_t contract, eosio::vm::span<const char> key, eosio::vm::span<const char> value, uint64_t payer) {
      return kv_get_db().kv_set(contract, key.data(), key.size(), value.data(), value.size(), payer);
   }

   bool kv_get(uint64_t contract, eosio::vm::span<const char> key, uint32_t* value_size) {
      return kv_get_db().kv_get(contract, key.data(), key.size(), *value_size);
   }

   uint32_t kv_get_data(uint32_t offset, eosio::vm::span<char> data) {
      return kv_get_db().kv_get_data(offset, data.data(), data.size());
   }

   uint32_t kv_it_create(uint64_t contract, eosio::vm::span<const char> prefix) {
      auto&    kdb = kv_get_db();
      uint32_t itr;
      if (!derived().get_db_view_state().kv_destroyed_iterators.empty()) {
         itr = derived().get_db_view_state().kv_destroyed_iterators.back();
         derived().get_db_view_state().kv_destroyed_iterators.pop_back();
      } else {
         // Sanity check in case the per-database limits are set poorly
         eosio::check(derived().get_db_view_state().kv_iterators.size() <= 0xFFFFFFFFu, "Too many iterators");
         itr = derived().get_db_view_state().kv_iterators.size();
         derived().get_db_view_state().kv_iterators.emplace_back();
      }
      derived().get_db_view_state().kv_iterators[itr] = kdb.kv_it_create(contract, prefix.data(), prefix.size());
      return itr;
   }

   void kv_it_destroy(uint32_t itr) {
      kv_check_iterator(itr);
      derived().get_db_view_state().kv_destroyed_iterators.push_back(itr);
      derived().get_db_view_state().kv_iterators[itr].reset();
   }

   int32_t kv_it_status(uint32_t itr) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(derived().get_db_view_state().kv_iterators[itr]->kv_it_status());
   }

   int32_t kv_it_compare(uint32_t itr_a, uint32_t itr_b) {
      kv_check_iterator(itr_a);
      kv_check_iterator(itr_b);
      return derived().get_db_view_state().kv_iterators[itr_a]->kv_it_compare(
            *derived().get_db_view_state().kv_iterators[itr_b]);
   }

   int32_t kv_it_key_compare(uint32_t itr, eosio::vm::span<const char> key) {
      kv_check_iterator(itr);
      return derived().get_db_view_state().kv_iterators[itr]->kv_it_key_compare(key.data(), key.size());
   }

   int32_t kv_it_move_to_end(uint32_t itr) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(derived().get_db_view_state().kv_iterators[itr]->kv_it_move_to_end());
   }

   int32_t kv_it_next(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(
            derived().get_db_view_state().kv_iterators[itr]->kv_it_next(found_key_size, found_value_size));
   }

   int32_t kv_it_prev(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(
            derived().get_db_view_state().kv_iterators[itr]->kv_it_prev(found_key_size, found_value_size));
   }

   int32_t kv_it_lower_bound(uint32_t itr, eosio::vm::span<const char> key, uint32_t* found_key_size,
                             uint32_t* found_value_size) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(derived().get_db_view_state().kv_iterators[itr]->kv_it_lower_bound(
            key.data(), key.size(), found_key_size, found_value_size));
   }

   int32_t kv_it_key(uint32_t itr, uint32_t offset, eosio::vm::span<char> dest, uint32_t* actual_size) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(
            derived().get_db_view_state().kv_iterators[itr]->kv_it_key(offset, dest.data(), dest.size(), *actual_size));
   }

   int32_t kv_it_value(uint32_t itr, uint32_t offset, eosio::vm::span<char> dest, uint32_t* actual_size) {
      kv_check_iterator(itr);
      return static_cast<int32_t>(derived().get_db_view_state().kv_iterators[itr]->kv_it_value(
            offset, dest.data(), dest.size(), *actual_size));
   }

   kv_context_rocksdb& kv_get_db() {
      return derived().get_db_view_state().kv_state;
   }

   void kv_check_iterator(uint32_t itr) {
      eosio::check(itr < derived().get_db_view_state().kv_iterators.size() &&
                         derived().get_db_view_state().kv_iterators[itr],
                   "Bad key-value iterator");
   }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_erase);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_set);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_get);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_get_data);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_create);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_destroy);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_status);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_compare);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_key_compare);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_move_to_end);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_next);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_prev);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_lower_bound);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_key);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, kv_it_value);
   }
}; // db_callbacks

class kv_environment : public db_callbacks<kv_environment> {
 public:
   using base = db_callbacks<kv_environment>;
   db_view_state& state;

   kv_environment(db_view_state& state) : state{ state } {}
   kv_environment(const kv_environment&) = default;

   auto& get_db_view_state() { return state; }

   int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) {
      return base::kv_erase(contract, { key, key_size });
   }

   int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size, uint64_t payer) {
      return base::kv_set(contract, { key, key_size }, { value, value_size }, payer);
   }

   void kv_set(uint64_t contract, const std::vector<char>& k, const std::vector<char>& v, uint64_t payer) {
      base::kv_set(contract, { k.data(), k.size() }, { v.data(), v.size() }, payer);
   }

   bool kv_get( uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) {
      return base::kv_get(contract, { key, key_size }, &value_size);
   }

   uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) {
      return base::kv_get_data(offset, { data, data_size });
   }

   uint32_t kv_it_create(uint64_t contract, const char* prefix, uint32_t prefix_size) {
      return base::kv_it_create(contract, { prefix, prefix_size });
   }

   int32_t kv_it_key_compare(uint32_t itr, const char* key, uint32_t key_size) {
      return base::kv_it_key_compare(itr, { key, key_size });
   }

   int32_t kv_it_lower_bound(uint32_t itr, const char* key, uint32_t key_size, uint32_t* found_key_size = nullptr,
                             uint32_t* found_value_size = nullptr) {
      return base::kv_it_lower_bound(itr, { key, key_size }, found_key_size, found_value_size);
   }

   int32_t kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t dest_size, uint32_t& actual_size) {
      return base::kv_it_key(itr, offset, { dest, dest_size }, &actual_size);
   }

   int32_t kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t dest_size, uint32_t& actual_size) {
      return base::kv_it_value(itr, offset, { dest, dest_size }, &actual_size);
   }
};

} // namespace b1::rodeos
