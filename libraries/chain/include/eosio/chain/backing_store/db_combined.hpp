#pragma once
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>

namespace eosio { namespace chain { namespace backing_store {
using session_type = eosio::session::session<eosio::session::session<eosio::session::rocksdb_t>>;
template<typename Object>
const char* contract_table_type();

// chainlib reserves prefixes 0x10 - 0x2F.
static constexpr char rocksdb_contract_kv_prefix = 0x11; // for KV API
static constexpr char rocksdb_contract_db_prefix = 0x12; // for DB API

/*
 * This is a helper for Receivers that expect only certain outcomes
 * and want to provide error handling for the remaining expected
 * callbacks
 */
template<typename Object>
class error_receiver {
public:
   void add_row(const Object& row, const chainbase::database& db) {
      wrong_row(contract_table_type<Object>());
   }
private:
   virtual void wrong_row(const char* type_received) = 0;
};
namespace detail {
   class manage_stack {
   public:
      manage_stack(const kv_undo_stack_ptr& kv_undo_stack) : kv_undo_stack_(kv_undo_stack), undo_(kv_undo_stack->empty()) {
         if (undo_) {
            // Get a session to iterate over.
            kv_undo_stack->push();
         }
      }

      ~manage_stack(){
         if (undo_) {
            kv_undo_stack_->undo();
         }
      }
   private:
      const kv_undo_stack_ptr& kv_undo_stack_;
      const bool undo_;
   };
}

template <typename F>
bool read_rocksdb_entry(const eosio::session::shared_bytes& actual_db_kv_key,
                        const eosio::session::shared_bytes& value,
                        F& function) {
   uint64_t    contract;
   constexpr std::size_t type_size = 1;
   std::size_t key_prefix_size = type_size + sizeof(contract);
   EOS_ASSERT(actual_db_kv_key.size() >= key_prefix_size, database_exception, "Unexpected key in rocksdb");

   auto key_buffer = std::vector<char>{ actual_db_kv_key.data(), actual_db_kv_key.data() + actual_db_kv_key.size() };
   auto begin = std::begin(key_buffer) + type_size;
   auto end = std::begin(key_buffer) + key_prefix_size;
   b1::chain_kv::extract_key(begin, end, contract);

   const char* post_contract = actual_db_kv_key.data() + key_prefix_size;
   const std::size_t remaining = actual_db_kv_key.size() - key_prefix_size;
   return function(contract, post_contract, remaining, value.data(), value.size());
};

template <typename F>
void walk_rocksdb_entries_with_prefix(const kv_undo_stack_ptr& kv_undo_stack,
                                      const eosio::session::shared_bytes& begin_key,
                                      const eosio::session::shared_bytes& end_key,
                                      F& function) {
   detail::manage_stack ms(kv_undo_stack);
   auto begin_it = kv_undo_stack->top().lower_bound(begin_key);
   const auto end_it = kv_undo_stack->top().lower_bound(end_key);
//   auto move_it = (begin_key < end_key ? [](auto& itr) { return ++itr; } :  [](auto& itr) { return --itr; } );
   bool keep_processing = true;
   for (auto it = begin_it; keep_processing && it != end_it; ++it) {
      // iterating through the session will always return a valid value
      keep_processing = read_rocksdb_entry((*it).first, *(*it).second, function);
   }
};

struct table_id_object_view {
   account_name code;  //< code should not be changed within a chainbase modifier lambda
   scope_name scope; //< scope should not be changed within a chainbase modifier lambda
   table_name table; //< table should not be changed within a chainbase modifier lambda
   account_name payer;
   uint32_t count = 0; /// the number of elements in the table
};

struct blob {
   std::string data;
};

template<typename DataStream>
inline DataStream &operator<<(DataStream &ds, const blob &b) {
   fc::raw::pack(ds, b.data);
   return ds;
}

template<typename DataStream>
inline DataStream &operator>>(DataStream &ds, blob &b) {
   fc::raw::unpack(ds, b.data);
   return ds;
}

struct primary_index_view {
   uint64_t primary_key;
   account_name payer;
   blob value;
};

template<typename SecondaryKeyType>
struct secondary_index_view {
   uint64_t primary_key;
   account_name payer;
   SecondaryKeyType secondary_key;
};

template<typename Receiver>
class add_database_receiver {
public:
   add_database_receiver(Receiver& receiver, const chainbase::database& db) : receiver_(receiver), db_(db) {}
   template<typename Object>
   void add_row(const Object& row) {
      receiver_.add_row(row, db_);
   }

   void add_row(fc::unsigned_int x){
      receiver_.add_row(x, db_);
   }

private:
   Receiver& receiver_;
   const chainbase::database& db_;
};

template<typename Receiver>
class rocksdb_whole_db_table_collector {
public:
   rocksdb_whole_db_table_collector(Receiver &r)
   : receiver_(r) {}

   void add_row(const chain::backing_store::primary_index_view& row){
      primary_indices_.emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row){
      secondary_indices<uint64_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row){
      secondary_indices<chain::uint128_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row){
      secondary_indices<chain::key256_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row){
      secondary_indices<float64_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row){
      secondary_indices<float128_t>().emplace_back(row);
   }

   void add_row(const chain::backing_store::table_id_object_view& row){
      receiver_.add_row(row);
   }

   void add_row(fc::unsigned_int x){
      auto add = [&rec=receiver_](const auto &row) { rec.add_row(row); };
      add(x);
      // now that the whole table has been found, now we can report the individual keys
      std::for_each(primary_indices_.begin(), primary_indices_.end(), add);
      primary_indices_.clear();
      std::tuple<uint64_t, uint128_t, key256_t, float64_t, float128_t> secondary_key_types;
      std::apply([this](auto... index) { (this->write_secondary_indices(index), ...); },
                 secondary_key_types);
   }
private:
   template<typename IndexType>
   auto& secondary_indices() {
      return std::get<std::vector<secondary_index_view<IndexType>>>(all_secondary_indices_);
   }

   template<typename IndexType>
   void write_secondary_indices(IndexType) {
      auto& indices = secondary_indices<IndexType>();
      auto add = [this](const auto& row) { receiver_.add_row(row); };
      add(fc::unsigned_int(indices.size()));
      std::for_each(indices.begin(), indices.end(), add);
      indices.clear();
   }

   Receiver& receiver_;
   std::vector<primary_index_view> primary_indices_;
   template<typename T>
   using secondary_index_views = std::vector<secondary_index_view<T>>;
   std::tuple<secondary_index_views<uint64_t>,
              secondary_index_views<uint128_t>,
              secondary_index_views<key256_t>,
              secondary_index_views<float64_t>,
              secondary_index_views<float128_t> >
         all_secondary_indices_;
};

auto process_all = []() { return true; };

enum class key_context { complete, standalone };
template<typename Receiver, typename Function = std::decay_t < decltype(process_all)>>
class rocksdb_contract_db_table_writer {
public:
   using key_type = db_key_value_format::key_type;

   rocksdb_contract_db_table_writer(Receiver &r, key_context context = key_context::complete, Function keep_processing = process_all)
         : receiver_(r), context_(context), keep_processing_(process_all)  {}

   void extract_primary_index(b1::chain_kv::bytes::const_iterator remaining,
                              b1::chain_kv::bytes::const_iterator key_end, const char *value,
                              std::size_t value_size) {
      uint64_t primary_key;
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      backing_store::payer_payload pp(value, value_size);
      receiver_.add_row(primary_index_view{primary_key, pp.payer, {std::string{pp.value, pp.value + pp.value_size}}});
      ++primary_count_;
   }

   template<typename IndexType>
   void extract_secondary_index(b1::chain_kv::bytes::const_iterator remaining,
                                b1::chain_kv::bytes::const_iterator key_end, const char *value,
                                std::size_t value_size) {
      IndexType secondary_key;
      uint64_t primary_key;
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, secondary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      backing_store::payer_payload pp(value, value_size);
      receiver_.add_row(secondary_index_view<IndexType>{primary_key, pp.payer, secondary_key});
      ++secondary_count<IndexType>();
   }

   bool operator()(uint64_t contract, const char *key, std::size_t key_size,
                   const char *value, std::size_t value_size) {
      if (!keep_processing_()) {
         return false;
      }
      b1::chain_kv::bytes composite_key(key, key + key_size);
      auto[scope, table, remaining, type] = backing_store::db_key_value_format::get_prefix_thru_key_type(
            composite_key);

      if (type == key_type::table) {
         backing_store::payer_payload pp(value, value_size);

         auto add_row = [this](const auto &row) { receiver_.add_row(row); };

         uint32_t total_count = primary_count_;
         for (auto& count : all_secondary_indices_count_) {
            total_count += count;
            count = 0;
         }

         add_row(table_id_object_view{account_name{contract}, scope, table, pp.payer, total_count});

         add_row(fc::unsigned_int(primary_count_));
         primary_count_ = 0;
      } else if (type != key_type::primary_to_sec) {
         if (context_ == key_context::standalone) {
            // for individual keys, need to report the table info for reference
            check_context(name{contract}, scope, table);
         }
         std::invoke(extract_index_member_fun_[static_cast<int>(type)], this, remaining, composite_key.end(),
                     value, value_size);
      }
      return true;
   }

private:
   template<typename IndexType>
   uint32_t& secondary_count() {
      constexpr auto index = static_cast<char>(db_key_value_format::derive_secondary_key_type<IndexType>()) -
                             static_cast<char>(db_key_value_format::key_type::sec_i64);
      return all_secondary_indices_count_[index];
   }

   void check_context(const name& code, const name& scope, const name& table) {
      if (table_context_ && table_context_->code == code &&
          table_context_->scope == scope && table_context_->table == table) {
         return;
      }
      table_context_ = table_id_object_view{code, scope, table, name{}, 0};
      receiver_.add_row(*table_context_);
   }

   Receiver &receiver_;
   const key_context context_;
   Function keep_processing_;
   std::optional<table_id_object_view> table_context_;

   using extract_index_member_fun_t = void (rocksdb_contract_db_table_writer::*)(
         b1::chain_kv::bytes::const_iterator, b1::chain_kv::bytes::const_iterator, const char *, std::size_t);
   constexpr static unsigned member_fun_count = static_cast<unsigned char>(key_type::sec_long_double) + 1;
   const static extract_index_member_fun_t extract_index_member_fun_[member_fun_count];
   uint32_t primary_count_ = 0;
   constexpr static unsigned secondary_indices = static_cast<char>(db_key_value_format::key_type::sec_long_double) -
                                                 static_cast<char>(db_key_value_format::key_type::sec_i64) + 1;
   uint32_t all_secondary_indices_count_[secondary_indices] = {};
};

// array mapping db_key_value_format::key_type values (except table) to functions to break down to component parts
template<typename Receiver, typename Function>
const typename rocksdb_contract_db_table_writer<Receiver, Function>::extract_index_member_fun_t
      rocksdb_contract_db_table_writer<Receiver, Function>::extract_index_member_fun_[] = {
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_primary_index,
   nullptr,                                                                      // primary_to_sec type is covered by writing the secondary key type
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<uint64_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<uint128_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<key256_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<float64_t>,
   &rocksdb_contract_db_table_writer<Receiver, Function>::extract_secondary_index<float128_t>,
};

template<typename Receiver, typename Function = std::decay_t < decltype(process_all)>>
class rocksdb_contract_kv_table_writer {
public:
   rocksdb_contract_kv_table_writer(Receiver& r, const Function& keep_processing = process_all)
         : receiver_(r), keep_processing_(keep_processing) {}

   bool operator()(uint64_t contract, const char* key, std::size_t key_size,
                   const char* value, std::size_t value_size) {
      if (!keep_processing_()) {
         return false;
      }
      // In KV RocksDB, payer and actual data are packed together.
      // Extract them.
      backing_store::payer_payload pp(value, value_size);
      kv_object_view row{name(contract),
                         {{key, key + key_size}},
                         {{pp.value, pp.value + pp.value_size}},
                         pp.payer};
      receiver_.add_row(row);
      return true;
   }

private:
   Receiver& receiver_;
   const Function& keep_processing_;
};

// will walk through all entries with the given prefix, so if passed an exact key, it will match that key
// and any keys with that key as a prefix
template <typename Receiver, typename Function = std::decay_t < decltype(process_all)>>
void walk_any_rocksdb_entries_with_prefix(const kv_undo_stack_ptr& kv_undo_stack,
                                          const eosio::session::shared_bytes& key,
                                          Receiver& receiver,
                                          Function keep_processing = process_all) {
   if (!key) {
      return;
   }
   const auto end_key = key.next_sub_key();
   const char prefix = key[0];
   if (prefix == rocksdb_contract_kv_prefix) {
      rocksdb_contract_kv_table_writer kv_writer(receiver, keep_processing);
      walk_rocksdb_entries_with_prefix(kv_undo_stack, key, end_key, kv_writer);
   }
   else {
      if (prefix != rocksdb_contract_db_prefix) {
         char buffer[10];
         const auto len = sprintf(buffer, "%20x", static_cast<uint8_t>(prefix));
         buffer[len] = '\0';
         FC_THROW_EXCEPTION(bad_composite_key_exception,
                            "Passed in key is prefixed with: ${prefix} which is neither the DB or KV prefix",
                            ("prefix", buffer));
      }

      rocksdb_contract_db_table_writer db_writer(receiver, keep_processing);
      walk_rocksdb_entries_with_prefix(kv_undo_stack, key, end_key, db_writer);
   }
}

// will walk through all entries with the given prefix, so if passed an exact key, it will match that key
// and any keys with that key as a prefix
template <typename Receiver, typename Session, typename Function = std::decay_t < decltype(process_all)>>
bool process_rocksdb_entry(const Session& session,
                           const eosio::session::shared_bytes& key,
                           Receiver& receiver) {
   if (!key) {
      return false;
   }
   const auto value = session.read(key);
   if (!value) {
      return false;
   }
   const char prefix = key[0];
   if (prefix == rocksdb_contract_kv_prefix) {
      rocksdb_contract_kv_table_writer kv_writer(receiver);
      read_rocksdb_entry(key, *value, kv_writer);
   }
   else {
      if (prefix != rocksdb_contract_db_prefix) {
         char buffer[10];
         const auto len = sprintf(buffer, "%20x", static_cast<uint8_t>(prefix));
         buffer[len] = '\0';
         FC_THROW_EXCEPTION(bad_composite_key_exception,
                            "Passed in key is prefixed with: ${prefix} which is neither the DB or KV prefix",
                            ("prefix", buffer));
      }

      rocksdb_contract_db_table_writer db_writer(receiver, key_context::standalone);
      read_rocksdb_entry(key, *value, db_writer);
   }
   return true;
}

template<typename Object>
const char* contract_table_type() {
   if constexpr (std::is_same_v<Object, kv_object_view>) {
      return "kv_object_view";
   }
   else if constexpr (std::is_same_v<Object, table_id_object_view>) {
      return "table_id_object_view";
   }
   else if constexpr (std::is_same_v<Object, primary_index_view>) {
      return "primary_index_view";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<uint64_t>>) {
      return "secondary_index_view<uint64_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<eosio::chain::uint128_t>>) {
      return "secondary_index_view<uint128_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<eosio::chain::key256_t>>) {
      return "secondary_index_view<key256_t>";
   }
   else if constexpr (std::is_same_v<Object, secondary_index_view<float64_t>>) {
      return "secondary_index_view<float64_t>";
   }
   else {
      static_assert(std::is_same_v<Object, secondary_index_view<float128_t>>);
      return "secondary_index_view<float128_t>";
   }
}

}}}

FC_REFLECT(eosio::chain::backing_store::table_id_object_view, (code)(scope)(table)(payer)(count) )
FC_REFLECT(eosio::chain::backing_store::primary_index_view, (primary_key)(payer)(value) )
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<uint64_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<eosio::chain::uint128_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<eosio::chain::key256_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<float64_t>)
REFLECT_SECONDARY(eosio::chain::backing_store::secondary_index_view<float128_t>)

namespace fc {
   inline
   void to_variant( const eosio::chain::backing_store::blob& b, variant& v ) {
      v = variant(base64_encode(b.data.data(), b.data.size()));
   }

   inline
   void from_variant( const variant& v, eosio::chain::backing_store::blob& b ) {
      b.data = base64_decode(v.as_string());
   }
}
