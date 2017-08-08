#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/chain_controller.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void apply_context::require_authorization(const types::AccountName& account) {
   auto itr = boost::find_if(msg.authorization, [&account](const auto& auth) { return auth.account == account; });
   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Transaction is missing required authorization from ${acct}", ("acct", account));
   used_authorizations[itr - msg.authorization.begin()] = true;
}

void apply_context::require_scope(const types::AccountName& account)const {
   auto itr = boost::find_if(trx.scope, [&account](const auto& scope) {
      return scope == account;
   });

   if( controller.should_check_scope() ) {
      EOS_ASSERT( itr != trx.scope.end(), tx_missing_scope,
                 "Required scope ${scope} not declared by transaction", ("scope",account) );
   }
}

void apply_context::require_recipient(const types::AccountName& account) {
   if (account == msg.code)
      return;

   auto itr = boost::find_if(notified, [&account](const auto& recipient) {
      return recipient == account;
   });

   if (itr == notified.end()) {
      notified.push_back(account);
   }
}

bool apply_context::all_authorizations_used() const {
   return boost::algorithm::all_of_equal(used_authorizations, true);
}

//
// i64 functions
//

int32_t apply_context::load_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            AccountName(*key) ) );
   if( obj == nullptr ) { return -1; }
   auto copylen =  std::min<size_t>(obj->value.size(),valuelen);
   if( copylen ) {
      obj->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::remove_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            AccountName(*key) ) );
   if( obj ) {
      mutable_db.remove( *obj );
      return 1;
   }
   return 0;
}

int32_t apply_context::store_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            AccountName(*key) ) );
   if( obj ) {
      mutable_db.modify( *obj, [&]( auto& o ) {
         o.value.assign(value, valuelen);
      });
      return 0;
   } else {
      mutable_db.create<key_value_object>( [&](auto& o) {
         o.scope = scope;
         o.code  = code;
         o.table = table;
         o.key   = *key;
         o.value.insert( 0, value, valuelen );
      });
      return 1;
   }
}

int32_t apply_context::update_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            AccountName(*key) ) );
   if( !obj ) return 0;

   mutable_db.modify( *obj, [&]( auto& o ) {
      if( valuelen > o.value.size() ) {
         o.value.resize(valuelen);
      }
      memcpy(o.value.data(), value, valuelen);
   });

   return 1;
}

int32_t apply_context::front_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.lower_bound( boost::make_tuple( scope, code, table, uint64_t(0) ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::back_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.upper_bound( boost::make_tuple( scope, code, table, uint64_t(-1) ) );

   if( std::distance(idx.begin(), itr) == 0 ) return -1;

   --itr;

   if( itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::next_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.lower_bound( boost::make_tuple( scope, code, table, *key ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ||
       uint64_t(itr->key) != *key ) return -1;

   ++itr;

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::previous_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.lower_bound( boost::make_tuple( scope, code, table, *key ) );

   if( itr == idx.end() ||
       itr == idx.begin() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ||
       uint64_t(itr->key) != *key ) return -1;

   --itr;

   if( itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::lower_bound_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.lower_bound( boost::make_tuple( scope, code, table, *key ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::upper_bound_i64( Name scope, Name code, Name table, uint64_t *key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key_value_index,by_scope_key>();
   auto itr = idx.upper_bound( boost::make_tuple( scope, code, table, *key ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table) return -1;

   *key = itr->key;
   auto copylen = std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

//
// i128i128 helper functions
//

template <typename T>
struct i128i128_Key {
   static const uint128_t& get_first(const key128x128_value_object& obj) {
      return obj.primary_key;
   }
   static const uint128_t& get_second(const key128x128_value_object& obj) {
      return obj.secondary_key;
   }
   static const uint128_t* get_first(uint128_t* primary, uint128_t* secondary) {
      return primary;
   }
   static const uint128_t* get_second(uint128_t* primary, uint128_t* secondary) {
      return secondary;
   }
};

template <>
struct i128i128_Key <by_scope_secondary> {
   static const uint128_t& get_first(const key128x128_value_object& obj) {
      return obj.secondary_key;
   }
   static const uint128_t& get_second(const key128x128_value_object& obj) {
      return obj.primary_key;
   }
   static const uint128_t* get_first(uint128_t* primary, uint128_t* secondary) {
      return secondary;
   }
   static const uint128_t* get_second(uint128_t* primary, uint128_t* secondary) {
      return primary;
   }
};

//
// i128i128 generic functions
//

template <typename T>
int32_t apply_context::back_i128i128( Name scope, Name code, Name table, uint128_t* primary,
                                      uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.upper_bound( boost::make_tuple( AccountName(scope),
                                                  AccountName(code),
                                                  table,
                                                  uint128_t(-1), uint128_t(-1) ) );

   if( std::distance(idx.begin(), itr) == 0 ) return -1;

   --itr;

   if( itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::front_i128i128( Name scope, Name code, Name table, uint128_t* primary,
                                       uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  uint128_t(0), uint128_t(0) ) );
   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::load_i128i128( Name scope, Name code, Name table, uint128_t* primary,
                                    uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   auto key = i128i128_Key<T>::get_first(primary, secondary);

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  *key, uint128_t(0) ) );
   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ||
       i128i128_Key<T>::get_first(*itr) != *key) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::next_i128i128( Name scope, Name code, Name table,
                     uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   auto key1 = i128i128_Key<T>::get_first(primary, secondary);
   auto key2 = i128i128_Key<T>::get_second(primary, secondary);

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  *key1, *key2 ) );
   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ||
       i128i128_Key<T>::get_first(*itr) != *key1 ||
       i128i128_Key<T>::get_second(*itr) != *key2 ) return -1;

   ++itr;

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::previous_i128i128( Name scope, Name code, Name table,
                     uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   auto key1 = i128i128_Key<T>::get_first(primary, secondary);
   auto key2 = i128i128_Key<T>::get_second(primary, secondary);

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  *key1, *key2 ) );
   if( itr == idx.end() ||
       itr == idx.begin() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ||
       i128i128_Key<T>::get_first(*itr) != *key1 ||
       i128i128_Key<T>::get_second(*itr) != *key2 ) return -1;

   --itr;

   if( itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::lower_bound_i128i128( Name scope, Name code, Name table,
                     uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   auto key = i128i128_Key<T>::get_first(primary, secondary);

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  *key, uint128_t(0) ) );
   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

template <typename T>
int32_t apply_context::upper_bound_i128i128( Name scope, Name code, Name table,
                     uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

   require_scope( scope );

   auto key = i128i128_Key<T>::get_first(primary, secondary);

   const auto& idx = db.get_index<key128x128_value_index,T>();
   auto itr = idx.upper_bound( boost::make_tuple( uint64_t(scope),
                                                  uint64_t(code),
                                                  uint64_t(table),
                                                  *key, (unsigned __int128)(-1) ) );
   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code  != code  ||
       itr->table != table ) return -1;

    *primary   = itr->primary_key;
    *secondary = itr->secondary_key;

    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

int32_t apply_context::remove_i128i128( Name scope, Name code, Name table, uint128_t *primary, uint128_t *secondary, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key128x128_value_object,by_scope_primary>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            *primary, *secondary) );
   if( obj ) {
      mutable_db.remove( *obj );
      return 1;
   }
   return 0;
}

int32_t apply_context::store_i128i128( Name scope, Name code, Name table, uint128_t *primary, uint128_t *secondary,
                                       char* value, uint32_t valuelen ) {
   require_scope( scope );
   const auto* obj = db.find<key128x128_value_object,by_scope_primary>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            *primary, *secondary ) );
   //idump(( *((fc::uint128_t*)primary)) );
   //idump(( *((fc::uint128_t*)secondary)) );
   if( obj ) {
      wlog( "modify" );
      mutable_db.modify( *obj, [&]( auto& o ) {
         o.value.assign(value, valuelen);
      });
      return 0;
   } else {
      wlog( "new" );
      mutable_db.create<key128x128_value_object>( [&](auto& o) {
         o.scope = scope;
         o.code  = code;
         o.table = table;
         o.primary_key = *primary;
         o.secondary_key = *secondary;
         o.value.insert( 0, value, valuelen );
      });
      return 1;
   }
}

int32_t apply_context::update_i128i128( Name scope, Name code, Name table, uint128_t *primary, uint128_t *secondary,
                                       char* value, uint32_t valuelen ) {
   require_scope( scope );
   const auto* obj = db.find<key128x128_value_object,by_scope_primary>( boost::make_tuple(
                                                            AccountName(scope),
                                                            AccountName(code),
                                                            AccountName(table),
                                                            *primary, *secondary ) );
   if( !obj ) {
      return 0;
   }

   mutable_db.modify( *obj, [&]( auto& o ) {
      if( valuelen > o.value.size() ) {
         o.value.resize(valuelen);
      }
      memcpy(o.value.data(), value, valuelen);
   });

   return 1;
}

//
// i128i128 primary functions
//

int32_t apply_context::load_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return load_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::front_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return front_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::back_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return back_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::next_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return next_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::previous_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return previous_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::lower_bound_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return lower_bound_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::upper_bound_primary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return upper_bound_i128i128<by_scope_primary>(scope, code, table, primary, secondary, value, valuelen);
}

//
// i128i128 secondary functions
//

int32_t apply_context::front_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return front_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::load_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return load_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::back_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return back_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::next_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return next_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::previous_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
  return previous_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::lower_bound_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return load_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

int32_t apply_context::upper_bound_secondary_i128i128( Name scope, Name code, Name table,
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {
   return upper_bound_i128i128<by_scope_secondary>(scope, code, table, primary, secondary, value, valuelen);
}

} } // namespace eos::chain
