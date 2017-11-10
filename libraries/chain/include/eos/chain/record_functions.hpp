/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/key_value_object.hpp>

namespace eosio { namespace chain {

/// find_tuple helper
template <typename T>
struct find_tuple {};
template <>
struct find_tuple<key_value_object> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys);
   }
};
template <>
struct find_tuple<keystr_value_object> {
   inline static auto get(name scope, name code, name table, std::string* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         keys->data());
   }
};
template <>
struct find_tuple<key128x128_value_object> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys, *(keys+1) );
   }
};
template <>
struct find_tuple<key64x64x64_value_object> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys, *(keys+1), *(keys+2) );
   }
};

/// key_helper helper
template <typename T>
struct key_helper {};

template <>
struct key_helper<key_value_object> {
   inline static void set(key_value_object& object, uint64_t* keys) {
      object.primary_key = *keys;
   }
   inline static void set(uint64_t* keys, const key_value_object& object) {
      *keys = object.primary_key;
   }
   inline static bool compare(const key_value_object& object, uint64_t* keys) {
      return object.primary_key == *keys;
   }
};
template <>
struct key_helper<keystr_value_object> {
   inline static void set(keystr_value_object& object, std::string* keys) {
      object.primary_key.assign(keys->data(), keys->size());
   }
   inline static void set(std::string* keys, const keystr_value_object& object) {
      keys->assign(object.primary_key.data(), object.primary_key.size());
   }
   inline static bool compare(const keystr_value_object& object, std::string* keys) {
      return !keys->compare(object.primary_key.c_str());
   }
};
template <>
struct key_helper<key128x128_value_object> {
   inline static auto set(key128x128_value_object& object, uint128_t* keys) {
      object.primary_key   = *keys;
      object.secondary_key = *(keys+1);
   }
   inline static auto set(uint128_t* keys, const key128x128_value_object& object) {
      *keys     = object.primary_key;
      *(keys+1) = object.secondary_key;
   }
   inline static bool compare(const key128x128_value_object& object, uint128_t* keys) {
      return object.primary_key == *keys && 
             object.secondary_key == *(keys+1);
   }
};
template <>
struct key_helper<key64x64x64_value_object> {
   inline static auto set(key64x64x64_value_object& object, uint64_t* keys) {
      object.primary_key   = *keys;
      object.secondary_key = *(keys+1);
      object.tertiary_key  = *(keys+2);
   }
   inline static auto set(uint64_t* keys, const key64x64x64_value_object& object) {
      *keys     = object.primary_key;
      *(keys+1) = object.secondary_key;
      *(keys+2) = object.tertiary_key;
   }
   inline static bool compare(const key64x64x64_value_object& object, uint64_t* keys) {
      return object.primary_key == *keys && 
             object.secondary_key == *(keys+1) &&
             object.tertiary_key == *(keys+2);
   }
};

/// load_record_tuple helper
template <typename T, typename Q>
struct load_record_tuple {};

template <>
struct load_record_tuple<key_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *keys);
   }
};
template <>
struct load_record_tuple<keystr_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, std::string* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), keys->data());
   }
};
template <>
struct load_record_tuple<key128x128_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *keys, uint128_t(0));
   }
};
template <>
struct load_record_tuple<key128x128_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *(keys+1), uint128_t(0));
   }
};
template <>
struct load_record_tuple<key64x64x64_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *keys, uint64_t(0), uint64_t(0));
   }
};
template <>
struct load_record_tuple<key64x64x64_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *(keys+1), uint64_t(0));
   }
};
template <>
struct load_record_tuple<key64x64x64_value_object, by_scope_tertiary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *(keys+2));
   }
};




//load_record_compare
template <typename ObjectType, typename Scope>
struct load_record_compare {
   inline static auto compare(const ObjectType& object, typename ObjectType::key_type* keys) {
      return typename ObjectType::key_type(object.primary_key) == *keys;
   }
};
template <>
struct load_record_compare<keystr_value_object, by_scope_primary> {
   inline static auto compare(const keystr_value_object& object, std::string* keys) {
      return !memcmp(object.primary_key.data(), keys->data(), keys->size());      
   }
};
template <>
struct load_record_compare<key128x128_value_object, by_scope_secondary> {
   inline static auto compare(const key128x128_value_object& object, uint128_t* keys) {
      return object.secondary_key == *(keys+1);
   }
};
template <>
struct load_record_compare<key64x64x64_value_object, by_scope_primary> {
   inline static auto compare(const key64x64x64_value_object& object, uint64_t* keys) {
      return object.primary_key == *keys;
   }
};
template <>
struct load_record_compare<key64x64x64_value_object, by_scope_secondary> {
   inline static auto compare(const key64x64x64_value_object& object, uint64_t* keys) {
      return object.secondary_key == *(keys+1);
   }
};
template <>
struct load_record_compare<key64x64x64_value_object, by_scope_tertiary> {
   inline static auto compare(const key64x64x64_value_object& object, uint64_t* keys) {
      return object.tertiary_key == *(keys+2);
   }
};

//front_record_tuple
template <typename ObjectType>
struct front_record_tuple {
   inline static auto get(name scope, name code, name table) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         typename ObjectType::key_type(0), typename ObjectType::key_type(0), typename ObjectType::key_type(0));
   }
};

template <>
struct front_record_tuple<key_value_object> {
   inline static auto get(name scope, name code, name table) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), uint64_t(0));
   }
};
template <>
struct front_record_tuple<keystr_value_object> {
   inline static auto get(name scope, name code, name table) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table));
   }
};

// //back_record_tuple
// template <typename ObjectType>
// struct back_record_tuple {
//    inline static auto get(name scope, name code, name table) {
//       return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
//          typename ObjectType::key_type(-1), typename ObjectType::key_type(-1), typename ObjectType::key_type(-1));
//    }   
// };

//next_record_tuple (same for previous)
template <typename T>
struct next_record_tuple {};

template <>
struct next_record_tuple<key_value_object> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys);
   }
};
template <>
struct next_record_tuple<keystr_value_object> {
   inline static auto get(name scope, name code, name table, std::string* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         keys->data());
   }
};
template <>
struct next_record_tuple<key128x128_value_object> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( uint64_t(scope), uint64_t(code), uint64_t(table),
          *keys, *(keys+1));
   }
};
template <>
struct next_record_tuple<key64x64x64_value_object> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( uint64_t(scope), uint64_t(code), uint64_t(table),
          *keys, *(keys+1), *(keys+2));
   }
};


//lower_bound_tuple
template <typename ObjectType, typename Scope>
struct lower_bound_tuple{};

template <typename ObjectType>
struct lower_bound_tuple<ObjectType, by_scope_primary> {
   inline static auto get(name scope, name code, name table, typename ObjectType::key_type* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys, typename ObjectType::key_type(0), typename ObjectType::key_type(0) );
   }
};
template <>
struct lower_bound_tuple<key_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), *keys);
   }
};
template <>
struct lower_bound_tuple<keystr_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, std::string* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table), keys->data());
   }
};
template <>
struct lower_bound_tuple<key128x128_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+1), uint128_t(0));
   }
};
template <>
struct lower_bound_tuple<key64x64x64_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+1), uint64_t(0));
   }
};
template <>
struct lower_bound_tuple<key64x64x64_value_object, by_scope_tertiary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+2));
   }
};

//upper_bound_tuple
template <typename ObjectType, typename Scope>
struct upper_bound_tuple{};

template <typename ObjectType>
struct upper_bound_tuple<ObjectType, by_scope_primary> {
   inline static auto get(name scope, name code, name table, typename ObjectType::key_type* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *keys, typename ObjectType::key_type(-1), typename ObjectType::key_type(-1) );
   }
};
template <>
struct upper_bound_tuple<keystr_value_object, by_scope_primary> {
   inline static auto get(name scope, name code, name table, std::string* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         keys->data());
   }
};
template <>
struct upper_bound_tuple<key128x128_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint128_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+1), uint128_t(-1) );
   }
};
template <>
struct upper_bound_tuple<key64x64x64_value_object, by_scope_secondary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+1), uint64_t(-1) );
   }
};
template <>
struct upper_bound_tuple<key64x64x64_value_object, by_scope_tertiary> {
   inline static auto get(name scope, name code, name table, uint64_t* keys) {
      return boost::make_tuple( account_name(scope), account_name(code), account_name(table),
         *(keys+2));
   }
};

} } // eosio::chain