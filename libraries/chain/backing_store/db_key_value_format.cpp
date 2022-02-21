#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/combined_database.hpp>
#include <algorithm>
#include <cstring>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   namespace detail {
      constexpr std::size_t key_size(key_type kt) {
         switch(kt) {
            case key_type::primary:
               return sizeof(uint64_t);
            case key_type::sec_i64:
               return sizeof(uint64_t);
            case key_type::sec_i128:
               return sizeof(eosio::chain::uint128_t);
            case key_type::sec_i256:
               return sizeof(key256_t);
            case key_type::sec_double:
               return sizeof(float64_t);
            case key_type::sec_long_double:
               return sizeof(float128_t);
            case key_type::table:
               return 0; // a table entry ends at the type and has no trailing sub-"keys"
            default:
               FC_THROW_EXCEPTION(bad_composite_key_exception,
                                  "DB intrinsic key-value store composite key is malformed, key_size should not be "
                                  "called with key_type: ${kt}", ("kt", to_string(kt)));
         }
      }

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
            case key_type::table: return "table end key";
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

      // NOTE: very limited use till redesign
      constexpr uint64_t db_type_and_code_size = detail::prefix_size<eosio::session::shared_bytes>() - detail::prefix_size<b1::chain_kv::bytes>(); // 1 (db type) + 8 (contract)
      static_assert(db_type_and_code_size == sizeof(char) + sizeof(name), "Some assumptions on formatting have been broken");
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

   b1::chain_kv::bytes create_prefix_type_key(name scope, name table, key_type kt) {
      static constexpr std::size_t no_key_size = 0;
      static constexpr std::size_t no_extension_size = 0;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, no_key_size, kt, no_extension_size);
      return composite_key;
   }

   b1::chain_kv::bytes create_table_key(name scope, name table) {
      // table key ends with the type, so just reuse method
      return create_prefix_type_key(scope, table, key_type::table);
   }

   eosio::session::shared_bytes create_table_key(const eosio::session::shared_bytes& prefix_key) {
      const std::size_t full_prefix_size = db_key_value_format::detail::prefix_size<eosio::session::shared_bytes>();
      EOS_ASSERT(prefix_key.size() >= full_prefix_size, bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, create_table_key expects to be given a full "
                 "key that has a full prefix leading up to the type");
      const auto new_kt = static_cast<char>(key_type::table);
      const char* new_kt_ptr = &new_kt;
      // already have everything in char format/order, so just need to assemble it
      return detail::assemble<2, eosio::session::shared_bytes>({rocksdb::Slice{prefix_key.data(), full_prefix_size},
                                                                rocksdb::Slice{new_kt_ptr, 1}});
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

   b1::chain_kv::bytes extract_legacy_key(const eosio::session::shared_bytes& complete_key) {
      const auto slice = extract_legacy_slice(complete_key);
      b1::chain_kv::bytes ret {slice.data(), slice.data() + slice.size()};
      return ret;
   }

   rocksdb::Slice extract_legacy_slice(const eosio::session::shared_bytes& full_key) {
      // offset into the whole key for the old postfix (after db type and contract)
      return {full_key.data() + detail::db_type_and_code_size,
              full_key.size() - detail::db_type_and_code_size};
   }

   eosio::session::shared_bytes create_full_key(const b1::chain_kv::bytes& composite_key, name code) {
      static const char db_type_prefix = make_rocksdb_contract_db_prefix();
      b1::chain_kv::bytes code_as_bytes;
      b1::chain_kv::append_key(code_as_bytes, code.to_uint64_t());
      auto ret = eosio::session::make_shared_bytes<std::string_view, 3>({std::string_view{&db_type_prefix, 1},
                                                                     std::string_view{code_as_bytes.data(),
                                                                                      code_as_bytes.size()},
                                                                     std::string_view{composite_key.data(),
                                                                                      composite_key.size()}});
      return ret;
   }

   eosio::session::shared_bytes create_full_key_prefix(const eosio::session::shared_bytes& full_key, end_of_prefix prefix_end) {
      auto calc_extension = [](end_of_prefix prefix_end) {
         switch (prefix_end) {
            case end_of_prefix::pre_type: return std::size_t(0);
            case end_of_prefix::at_type: return std::size_t(1);
            case end_of_prefix::at_prim_to_sec_type: return std::size_t(2);
            case end_of_prefix::at_prim_to_sec_primary_key: return 2 + sizeof(uint64_t);
         }
      };
      const std::size_t extension_size = calc_extension(prefix_end);
      const std::size_t full_prefix_size = db_key_value_format::detail::prefix_size<eosio::session::shared_bytes>() + extension_size;
      EOS_ASSERT( full_prefix_size < full_key.size(), db_rocksdb_invalid_operation_exception,
                  "invariant failure in prefix_bundle, the passed in full_key was: ${size1} bytes, but it needs to "
                  "be at least: ${size2}", ("size1", full_key.size())("size2", full_prefix_size));
      return eosio::session::shared_bytes(full_key.data(), full_prefix_size);
   }

   full_key_data parse_full_key(const eosio::session::shared_bytes& full_key) {
      static const char db_type_prefix = make_rocksdb_contract_db_prefix();
      EOS_ASSERT( full_key.size() >= 1, db_rocksdb_invalid_operation_exception,
                  "parse_full_key was passed an empty key.");
      full_key_data data;
      const char* offset = full_key.data();
      data.db_type = *(offset++);
      const std::size_t db_type_size = 1;
      if (data.db_type != db_type_prefix || full_key.size() == db_type_size) {
         return data;
      }
      const std::size_t full_prefix_size = db_key_value_format::detail::prefix_size<eosio::session::shared_bytes>();
      const std::size_t full_prefix_type_size = full_prefix_size + sizeof(key_type);
      const std::size_t db_type_and_contract_size = db_type_size + sizeof(name);
      EOS_ASSERT( full_key.size() >= db_type_and_contract_size, db_rocksdb_invalid_operation_exception,
                  "parse_full_key was passed a key with a db type and erroneous data trailing.");
      const b1::chain_kv::bytes composite_contract_key {offset, offset + sizeof(name)};
      offset += sizeof(name);
      auto composite_loc = composite_contract_key.cbegin();
      uint64_t contract;
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_contract_key.cend(), contract), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the string size is"
                 " less than the sizeof(uint64_t), which was verified that it was not");
      EOS_ASSERT( composite_loc == composite_contract_key.cend(), db_rocksdb_invalid_operation_exception,
                  "bytes constructed to extract contract was constructed incorrectly.");
      data.contract = name{contract};
      const auto remaining = full_key.size() - db_type_and_contract_size;
      if (!remaining) {
         return data;
      }
      data.legacy_key = b1::chain_kv::bytes {offset, offset + remaining};
      auto prefix_thru_kt = get_prefix_thru_key_type(*data.legacy_key);
      data.scope = std::get<0>(prefix_thru_kt);
      data.table = std::get<1>(prefix_thru_kt);
      data.kt = std::get<3>(prefix_thru_kt);
      const std::size_t type_prefix_length = std::distance(data.legacy_key->cbegin(), std::get<2>(prefix_thru_kt));
      data.type_prefix = {data.legacy_key->data(), type_prefix_length};

      return data;
   }

   eosio::session::shared_bytes create_full_primary_key(name code, name scope, name table, uint64_t primary_key) {
      bytes composite_key = create_primary_key(scope, table, primary_key);
      return create_full_key(composite_key, code);
   }

   eosio::session::shared_bytes create_full_prefix_key(name code, name scope, name table, std::optional<key_type> kt) {
      bytes composite_key = kt ? create_prefix_type_key(scope, table, *kt) : create_prefix_key(scope, table);
      return create_full_key(composite_key, code);
   }
}}}} // namespace eosio::chain::backing_store::db_key_value_format
