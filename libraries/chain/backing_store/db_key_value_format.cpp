#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <cstring>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   namespace detail {
      b1::chain_kv::bytes prepare_composite_key_prefix(name scope, name table, std::size_t type_size, std::size_t key_size, std::size_t extension_size) {
         constexpr static auto scope_size = sizeof(scope);
         constexpr static auto table_size = sizeof(table);
         b1::chain_kv::bytes key_storage;
         key_storage.reserve(scope_size + table_size + type_size + key_size + extension_size);
         b1::chain_kv::append_key(key_storage, scope.to_uint64_t());
         b1::chain_kv::append_key(key_storage, table.to_uint64_t());
         return key_storage;
      }

      b1::chain_kv::bytes prepare_composite_key(name scope, name table, std::size_t key_size, key_type kt, std::size_t extension_size) {
         constexpr static auto type_size = sizeof(key_type);
         static_assert( type_size == 1, "" );
         auto key_storage = prepare_composite_key_prefix(scope, table, type_size, key_size, extension_size);
         key_storage.push_back(static_cast<char>(kt));
         return key_storage;
      }

      std::string to_string(const key_type& kt) {
         switch (kt) {
            case key_type::primary: return "primary key of type uint64_t";
            case key_type::primary_to_sec: return "primary key to secondary key type";
            case key_type::sec_i64: return "secondary key of type uint64_t";
            case key_type::sec_i128: return "secondary key of type uint128_t";
            case key_type::sec_i256: return "secondary key of type key256_t";
            case key_type::sec_double: return "secondary key of type float64_t";
            case key_type::sec_long_double: return "secondary key of type float128_t";
            default:
               const int kt_as_int = static_cast<char>(kt);
               return std::string("<invalid key_type: ") + std::to_string(kt_as_int) + ">";
         }
      }

      intermittent_decomposed_prefix_values extract_from_composite_key_prefix(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end) {
         auto key_loc = begin;
         uint64_t scope_name = 0;
         EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, scope_name), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain a scope");
         uint64_t table_name = 0;
         EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, table_name), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain a table");
         return { name{scope_name}, name{table_name}, key_loc };
      }

      intermittent_decomposed_values extract_from_composite_key(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end) {
         auto intermittent = extract_from_composite_key_prefix(begin, key_end);
         auto& key_loc = std::get<2>(intermittent);
         EOS_ASSERT(key_loc != key_end, bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain an indication of the "
                    "type of the db-key (primary uint64_t or secondary uint64_t/uint128_t/etc)");
         const key_type kt{*key_loc++};
         return { std::get<0>(intermittent), std::get<1>(intermittent), key_loc, kt };
      }
   } // namespace detail

   b1::chain_kv::bytes create_primary_key(name scope, name table, uint64_t primary_key) {
      const std::size_t zero_extension_size = 0;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, sizeof(uint64_t), key_type::primary, zero_extension_size);
      b1::chain_kv::append_key(composite_key, primary_key);
      return composite_key;
   }

   bool get_primary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& primary_key) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      if (kt != key_type::primary) {
         return false;
      }
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a primary key");
      EOS_ASSERT(composite_loc == composite_key.cend(), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it has extra data after the primary key");
      return true;
   }

   b1::chain_kv::bytes create_table_key(name scope, name table) {
      // table key ends with the type, so just reuse method
      return create_prefix_type_key<key_type::table>(scope, table);
   }

   bool get_table_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::primary;
      std::tie(scope, table, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      if (kt != key_type::table) {
         return false;
      }
      EOS_ASSERT(composite_loc == composite_key.cend(), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, there should be no trailing primary/secondary key");
      return true;
   }

   b1::chain_kv::bytes create_prefix_key(name scope, name table) {
      static constexpr std::size_t no_type_size = 0;
      static constexpr std::size_t no_key_size = 0;
      static constexpr std::size_t no_extension_size = 0;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key_prefix(scope, table, no_type_size, no_key_size, no_extension_size);
      return composite_key;
   }

   void get_prefix_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      std::tie(scope, table, composite_loc) = detail::extract_from_composite_key_prefix(composite_key.cbegin(), composite_key.cend());
   }

   bool get_trailing_primary_key(const rocksdb::Slice& full_key, const rocksdb::Slice& secondary_key_prefix, uint64_t& primary_key) {
      const auto sec_prefix_size = secondary_key_prefix.size();
      EOS_ASSERT(full_key.size() == sec_prefix_size + sizeof(primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_primary_key was passed a full key size: ${s1} bytes that was not "
                 "exactly ${s2} bytes (the size of a primary key) larger than the secondary_key_prefix size: ${s3}",
                 ("s1", full_key.size())("s2", sizeof(primary_key))("s3", sec_prefix_size));
      const auto comp = memcmp(secondary_key_prefix.data(), full_key.data(), sec_prefix_size);
      if (comp != 0) {
         return false;
      }
      auto start_offset = full_key.data() + sec_prefix_size;
      const b1::chain_kv::bytes composite_primary_key {start_offset, start_offset + sizeof(primary_key)};
      auto composite_loc = composite_primary_key.cbegin();
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_primary_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the string size is"
                 " less than the sizeof(uint64_t)");
      return true;
   }

   key_type extract_key_type(const b1::chain_kv::bytes& composite_key) {
      key_type kt = key_type::primary;
      std::tie(std::ignore, std::ignore, std::ignore, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      return kt;
   }

   key_type extract_primary_to_sec_key_type(const b1::chain_kv::bytes& composite_key) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::primary;
      std::tie(std::ignore, std::ignore, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      EOS_ASSERT(kt == key_type::primary_to_sec, bad_composite_key_exception,
                 "DB intrinsic extract_primary_to_sec_key_type was passed a key that was not of type: ${type}", ("type", detail::to_string(kt)));
      EOS_ASSERT(composite_loc != composite_key.cend(), bad_composite_key_exception,
                 "DB intrinsic extract_primary_to_sec_key_type was passed a key that was only the type prefix");
      const key_type sec_kt{*composite_loc++};
      return sec_kt;
   }

   intermittent_decomposed_values get_prefix_thru_key_type(const b1::chain_kv::bytes& composite_key) {
      return detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
   }
}}}} // namespace eosio::chain::backing_store::db_key_value_format
