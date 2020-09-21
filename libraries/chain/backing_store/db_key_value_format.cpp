#include <eosio/chain/backing_store/db_key_value_format.hpp>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   namespace detail {
      std::string to_string(const key_type& kt) {
         switch (kt) {
            case key_type::primary: return "primary key of type uint64_t";
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

      intermittent_decomposed_values extract_from_composite_key(eosio::session::shared_bytes::iterator begin,
                                                                eosio::session::shared_bytes::iterator key_end) {
         auto     key_loc    = begin;
         uint64_t scope_name = 0;
         EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, scope_name), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain a scope");
         uint64_t table_name = 0;
         EOS_ASSERT(b1::chain_kv::extract_key(key_loc, key_end, table_name), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain a table");
         EOS_ASSERT(key_loc != key_end, bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it does not contain an indication of the "
                    "type of the db-key (primary uint64_t or secondary uint64_t/uint128_t/etc)");
         return { name{ scope_name }, name{ table_name }, key_loc, key_type(*key_loc++) };
      }
   } // namespace detail

   eosio::session::shared_bytes create_primary_key(name scope, name table, uint64_t db_key) {
      auto buffer = detail::prepare_composite_key<uint64_t, key_type::primary>(scope, table, sizeof(uint64_t));
      b1::chain_kv::append_key(buffer, db_key);
      return eosio::session::make_shared_bytes(buffer.data(), buffer.size());
   }

   void get_primary_key(const eosio::session::shared_bytes& composite_key, name& scope, name& table, uint64_t& db_key) {
      eosio::session::shared_bytes::iterator composite_loc;
      key_type                               kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) =
            detail::extract_from_composite_key(composite_key.begin(), composite_key.end());
      EOS_ASSERT(kt == key_type::primary, bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to be a primary key, "
                 "but it is a: " +
                       detail::to_string(kt));
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.end(), db_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a primary key");
   }

}}}} // namespace eosio::chain::backing_store::db_key_value_format
