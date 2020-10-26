#pragma once
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>

namespace eosio { namespace chain { namespace backing_store {

template <typename F>
void walk_rocksdb_entries_with_prefix(const kv_undo_stack_ptr& kv_undo_stack, const std::vector<char>& prefix, F&& function) {
   const auto undo = kv_undo_stack->empty();
   if (undo) {
      // Get a session to iterate over.
      kv_undo_stack->push();
   }
   const auto prefix_key = eosio::session::shared_bytes(prefix.data(), prefix.size());
   const auto next_prefix = prefix_key.next();
   auto begin_it = kv_undo_stack->top().lower_bound(prefix_key);
   auto end_it = kv_undo_stack->top().lower_bound(next_prefix);
   for (auto it = begin_it; it != end_it; ++it) {
      auto key = (*it).first;

      uint64_t    contract;
      std::size_t key_prefix_size = prefix_key.size() + sizeof(contract);
      EOS_ASSERT(key.size() >= key_prefix_size, database_exception, "Unexpected key in rocksdb");

      auto key_buffer = std::vector<char>{ key.data(), key.data() + key.size() };
      auto begin = std::begin(key_buffer) + prefix_key.size();
      auto end = std::begin(key_buffer) + key_prefix_size;
      b1::chain_kv::extract_key(begin, end, contract);

      auto value = (*it).second;

      const char* post_contract = key.data() + key_prefix_size;
      const std::size_t remaining = key.size() - key_prefix_size;
      function(contract, post_contract, remaining, value->data(), value->size());
   }

   if (undo) {
      kv_undo_stack->undo();
   }
}

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
class rocksdb_contract_db_table_writer {
public:
   using key_type = eosio::chain::backing_store::db_key_value_format::key_type;

   rocksdb_contract_db_table_writer(Receiver &r, const chainbase::database &db,
                                    eosio::session::undo_stack<rocks_db_type> &kus)
         : receiver_(r), db_(db), kv_undo_stack_(kus) {}

   void extract_primary_index(b1::chain_kv::bytes::const_iterator remaining,
                              b1::chain_kv::bytes::const_iterator key_end, const char *value,
                              std::size_t value_size) {
      uint64_t primary_key;
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      backing_store::payer_payload pp(value, value_size);
      primary_indices_.emplace_back(
            primary_index_view{primary_key, pp.payer, {std::string{pp.value, pp.value + pp.value_size}}});
   }

   template<typename IndexType>
   auto &secondary_indices() {
      return std::get<std::vector<secondary_index_view<IndexType>>>(all_secondary_indices_);
   }

   template<typename IndexType>
   void extract_secondary_index(b1::chain_kv::bytes::const_iterator remaining,
                                b1::chain_kv::bytes::const_iterator key_end, const char *value,
                                std::size_t value_size) {
      auto &indices = secondary_indices<IndexType>();
      IndexType secondary_key;
      uint64_t primary_key;
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, secondary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed");
      backing_store::payer_payload pp(value, value_size);
      indices.emplace_back(secondary_index_view<IndexType>{primary_key, pp.payer, secondary_key});
   }

   template<typename IndexType>
   void write_secondary_indices(IndexType) {
      auto &indices = secondary_indices<IndexType>();
      auto add_row = [this](const auto &row) { receiver_.add_row(row, db_); };
      add_row(fc::unsigned_int(indices.size()));
      std::for_each(indices.begin(), indices.end(), add_row);
      indices.clear();
   }

   void operator()(uint64_t contract, const char *key, std::size_t key_size,
                   const char *value, std::size_t value_size) {

      b1::chain_kv::bytes composite_key(key, key + key_size);
      auto[scope, table, remaining, type] = backing_store::db_key_value_format::get_prefix_thru_key_type(
            composite_key);

      if (type == key_type::table) {
         backing_store::payer_payload pp(value, value_size);

         auto add_row = [this](const auto &row) { receiver_.add_row(row, db_); };

         std::tuple<uint64_t, uint128_t, key256_t, float64_t, float128_t> secondary_key_types;
         uint32_t total_count = primary_indices_.size();
         std::apply([this, &total_count](auto... index) {
            total_count += ( this->secondary_indices<decltype(index)>().size() + ...);
         }, secondary_key_types);

         add_row(table_id_object_view{account_name{contract}, scope, table, pp.payer, total_count});

         add_row(fc::unsigned_int(primary_indices_.size()));
         std::for_each(primary_indices_.begin(), primary_indices_.end(), add_row);
         primary_indices_.clear();
         std::apply([this](auto... index) { (this->write_secondary_indices(index), ...); },
                    secondary_key_types);
      } else if (type != key_type::primary_to_sec) {
         std::invoke(extract_index_member_fun_[static_cast<int>(type)], this, remaining, composite_key.end(),
                     value, value_size);
      }
   }

private:
   Receiver &receiver_;
   const chainbase::database &db_;
   eosio::session::undo_stack<rocks_db_type> &kv_undo_stack_;
   std::vector<primary_index_view> primary_indices_;
   template<typename T>
   using secondary_index_views = std::vector<secondary_index_view<T>>;
   std::tuple<secondary_index_views<uint64_t>,
         secondary_index_views<uint128_t>,
         secondary_index_views<key256_t>,
         secondary_index_views<float64_t>,
         secondary_index_views<float128_t> >
         all_secondary_indices_;

   using extract_index_member_fun_t = void (rocksdb_contract_db_table_writer::*)(
         b1::chain_kv::bytes::const_iterator, b1::chain_kv::bytes::const_iterator, const char *, std::size_t);
   constexpr static unsigned member_fun_count = static_cast<unsigned char>(key_type::sec_long_double) + 1;
   const static extract_index_member_fun_t extract_index_member_fun_[member_fun_count];
};

// array mapping db_key_value_format::key_type values (except table) to functions to break down to component parts
template<typename Receiver>
const typename rocksdb_contract_db_table_writer<Receiver>::extract_index_member_fun_t rocksdb_contract_db_table_writer<Receiver>::extract_index_member_fun_[] = {
      &rocksdb_contract_db_table_writer<Receiver>::extract_primary_index,
      nullptr,                                                                      // primary_to_sec type is covered by writing the secondary key type
      &rocksdb_contract_db_table_writer<Receiver>::extract_secondary_index<uint64_t>,
      &rocksdb_contract_db_table_writer<Receiver>::extract_secondary_index<uint128_t>,
      &rocksdb_contract_db_table_writer<Receiver>::extract_secondary_index<key256_t>,
      &rocksdb_contract_db_table_writer<Receiver>::extract_secondary_index<float64_t>,
      &rocksdb_contract_db_table_writer<Receiver>::extract_secondary_index<float128_t>,
};
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

// chainlib reserves prefixes 0x10 - 0x2F.
static const std::vector<char> rocksdb_contract_kv_prefix{ 0x11 }; // for KV API
static const std::vector<char> rocksdb_contract_db_prefix{ 0x12 }; // for DB API