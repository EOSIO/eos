#include <eosio/chain/db_key_value_format.hpp>

namespace eosio { namespace chain {
   std::string db_key_value_format::to_string(const key_type& kt) {
      switch (kt) {
         case key_type::primary: return "primary key of type uint64_t";
         case key_type::sec_i64: return "seconday key of type uint64_t";
         case key_type::sec_i128: return "seconday key of type uint128_t";
         case key_type::sec_i256: return "seconday key of type key256_t";
         case key_type::sec_double: return "seconday key of type float64_t";
         case key_type::sec_long_double: return "seconday key of type float128_t";
         default:
            const int kt_as_int = static_cast<char>(kt);
            return std::string("<invalid key_type: ") + std::to_string(kt_as_int) + ">";
      }
   }

   db_key_value_format::key_type db_key_value_format::extract_from_composite_key(b1::chain_kv::bytes::const_iterator& key_loc, b1::chain_kv::bytes::const_iterator key_end, name& scope, name& table) {
      uint64_t temp_name = 0;
      EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, temp_name), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it does not contain a scope");
      scope = name{temp_name};
      EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, temp_name), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it does not contain a table");
      table = name{temp_name};
      EOS_ASSERT(key_loc != key_end, bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it does not contain an indication of the "
                 "type of the db-key (primary uint64_t or secondary uint64_t/uint128_t/etc)");
      return key_type(*key_loc++);
   }

   b1::chain_kv::bytes db_key_value_format::create_primary_key(name scope, name table, uint64_t db_key) {
      b1::chain_kv::bytes composite_key;
      prepare_composite_key<uint64_t, key_type::primary>(composite_key, scope, table, sizeof(uint64_t));
      b1::chain_kv::append_key(composite_key, db_key);
      return composite_key;
   }

   void db_key_value_format::get_primary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& db_key) {
      b1::chain_kv::bytes::const_iterator composite_loc = composite_key.cbegin();
      auto kt = extract_from_composite_key(composite_loc, composite_key.cend(), scope, table);
      EOS_ASSERT(kt == key_type::primary, bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to be a primary key, "
                 "but it is a: " + to_string(kt));
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), db_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a primary key");
   }

}} // namespace eosio::chain
