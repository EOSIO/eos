#pragma once
#include <eosio/chain/contract_table_objects.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>

namespace eosio { namespace chain { namespace backing_store {

template<typename>
struct array_size;

template<typename T, size_t N>
struct array_size< std::array<T,N> > {
    static constexpr size_t size = N;
};

template <typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst, typename Enable = void>
class db_secondary_key_helper;

template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
class db_secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
   typename std::enable_if<std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value>::type >
{
   public:
      typedef SecondaryKey secondary_key_type;

      static void set(secondary_key_type& sk_in_table, const secondary_key_type& sk_from_wasm) {
         sk_in_table = sk_from_wasm;
      }

      static void get(secondary_key_type& sk_from_wasm, const secondary_key_type& sk_in_table ) {
         sk_from_wasm = sk_in_table;
      }

      static auto create_tuple(const table_id_object& tab, const secondary_key_type& secondary) {
         return boost::make_tuple( tab.id, secondary );
      }
};

template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
class db_secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
   typename std::enable_if<!std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value &&
                           std::is_pointer<typename std::decay<SecondaryKeyProxy>::type>::value>::type >
{
   public:
      typedef SecondaryKey      secondary_key_type;
      typedef SecondaryKeyProxy secondary_key_proxy_type;
      typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

      static constexpr size_t N = array_size<SecondaryKey>::size;

      static void set(secondary_key_type& sk_in_table, secondary_key_proxy_const_type sk_from_wasm) {
         std::copy(sk_from_wasm, sk_from_wasm + N, sk_in_table.begin());
      }

      static void get(secondary_key_proxy_type sk_from_wasm, const secondary_key_type& sk_in_table) {
         std::copy(sk_in_table.begin(), sk_in_table.end(), sk_from_wasm);
      }

      static auto create_tuple(const table_id_object& tab, secondary_key_proxy_const_type sk_from_wasm) {
         secondary_key_type secondary;
         std::copy(sk_from_wasm, sk_from_wasm + N, secondary.begin());
         return boost::make_tuple( tab.id, secondary );
      }
};

}}} // namespace eosio::chain::backing_store
