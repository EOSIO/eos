#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/chain_controller.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void message_validate_context::require_authorization(const types::AccountName& account) {
#warning TODO: Look up the permission_object that account has specified to use for this message type
   if (authChecker)
      EOS_ASSERT(authChecker->requirePermission({account, "active"}), tx_missing_auth,
                 "Transaction does not declare required authority '${auth}'", ("auth", account));
}

void message_validate_context::require_scope(const types::AccountName& account)const {
   auto itr = boost::find_if(trx.scope, [&account](const auto& scope) {
      return scope == account;
   });

   if( controller.should_check_scope() ) {
      EOS_ASSERT( itr != trx.scope.end(), tx_missing_scope,
                 "Required scope ${scope} not declared by transaction", ("scope",account) );
   }
}

bool apply_context::has_recipient( const types::AccountName& account )const {
   if( msg.code == account ) return true;

   auto itr = boost::find_if(notified, [&account](const auto& recipient) {
      return recipient == account;
   });

   return itr != notified.end();
}

void apply_context::require_recipient(const types::AccountName& account) {
   if( !has_recipient( account ) ) {
      notified.push_back(account);
   }
}

int32_t message_validate_context::load_i64( Name scope, Name code, Name table, Name key, char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope), 
                                                            AccountName(code), 
                                                            AccountName(table), 
                                                            AccountName(key) ) );
   if( obj == nullptr ) { return -1; }
   auto copylen =  std::min<size_t>(obj->value.size(),valuelen);
   if( copylen ) {
      obj->value.copy(value, copylen);
   }
   return copylen;
}

int32_t message_validate_context::back_primary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

   return -1;
   /*
    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_primary>();
    auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                  uint64_t(code), 
                                                  table.value+1, 
                                                  *primary, uint128_t(0) ) );

    if( itr == idx.begin() && itr == idx.end() ) 
       return 0;

    --itr;

    if( itr->scope != scope ||
        itr->code != code ||
        itr->table != table ||
        itr->primary_key != *primary ) return -1;

    *secondary = itr->secondary_key;
    *primary = itr->primary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
    */
}

int32_t message_validate_context::back_secondary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_secondary>();
    auto itr = idx.lower_bound( boost::make_tuple( AccountName(scope), 
                                                  AccountName(code), 
                                                  table.value+1, 
                                                  *secondary, uint128_t(0) ) );

    if( itr == idx.end() ) 
       return -1;

    --itr;

    if( itr->scope != scope ||
        itr->code != code ||
        itr->table != table 
        ) return -2;

    *secondary = itr->secondary_key;
    *primary = itr->primary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}


int32_t message_validate_context::front_primary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_primary>();
    auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                  uint64_t(code), 
                                                  uint64_t(table), 
                                                  *primary, uint128_t(0) ) );

    if( itr == idx.end() ) 
       return -1;

    --itr;

    if( itr->scope != scope ||
        itr->code != code ||
        itr->table != table 
       ) return -2;

    *secondary = itr->secondary_key;
    *primary = itr->primary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}
int32_t message_validate_context::front_secondary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_secondary>();
    auto itr = idx.lower_bound( boost::make_tuple( AccountName(scope), 
                                                  AccountName(code), 
                                                  AccountName(table), 
                                                  uint128_t(0), uint128_t(0) ) );

    FC_ASSERT( itr == idx.begin(), "lower bound of all 0 should be begin" );

    if( itr == idx.end() )  {
       return -1;
    }


    if( itr->scope != scope ||
        itr->code != code ||
        itr->table != table ) {
        return -2;  
    }

    *secondary = itr->secondary_key;
    *primary = itr->primary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}


int32_t message_validate_context::load_primary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_primary>();
    auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                  uint64_t(code), 
                                                  uint64_t(table), 
                                                  *primary, uint128_t(0) ) );

    if( itr == idx.end() ||
        itr->scope != (scope) ||
        itr->code != (code) ||
        itr->table != (table) ||
        itr->primary_key != *primary ) return -1;

    *secondary = itr->secondary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

int32_t message_validate_context::load_secondary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

    require_scope( scope );
    const auto& idx = db.get_index<key128x128_value_index,by_scope_secondary>();
    auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                   uint64_t(code), 
                                                   uint64_t(table), 
                                                  *secondary, uint128_t(0) ) );

    if( itr == idx.end() ||
        itr->scope != (scope) ||
        itr->code != (code) ||
        itr->table != (table) ||
        itr->secondary_key != *secondary ) return -1;

    *primary = itr->primary_key;
    
    auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
    if( copylen ) {
       itr->value.copy(value, copylen);
    }
    return copylen;
}

int32_t message_validate_context::lowerbound_primary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

   require_scope( scope );
   const auto& idx = db.get_index<key128x128_value_index,by_scope_primary>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                  uint64_t(code), 
                                                  uint64_t(table), 
                                                  *primary, uint128_t(0) ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code != code ||
       itr->table != table ) return -1;

   *primary   = itr->primary_key;
   *secondary = itr->secondary_key;

   auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t message_validate_context::lowerbound_secondary_i128i128( Name scope, Name code, Name table, 
                                 uint128_t* primary, uint128_t* secondary, char* value, uint32_t valuelen ) {

   require_scope( scope );
   const auto& idx = db.get_index<key128x128_value_index,by_scope_secondary>();
   auto itr = idx.lower_bound( boost::make_tuple( uint64_t(scope), 
                                                  uint64_t(code), 
                                                  uint64_t(table), 
                                                  uint128_t(0), *secondary  ) );

   if( itr == idx.end() ||
       itr->scope != scope ||
       itr->code != code ||
       itr->table != table ) return -1;

   *primary   = itr->primary_key;
   *secondary = itr->secondary_key;

   auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
   if( copylen ) {
      itr->value.copy(value, copylen);
   }
   return copylen;
}

int32_t apply_context::remove_i128i128( Name scope, Name table, uint128_t primary, uint128_t secondary ) {
   require_scope( scope );

   const auto* obj = db.find<key128x128_value_object,by_scope_primary>( boost::make_tuple(
                                                            AccountName(scope), 
                                                            AccountName(code), 
                                                            AccountName(table), 
                                                            primary, secondary) );
   if( obj ) {
      mutable_db.remove( *obj );
      return 1;
   }
   return 0;
}

int32_t apply_context::remove_i64( Name scope, Name table, Name key ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope), 
                                                            AccountName(code), 
                                                            AccountName(table), 
                                                            AccountName(key) ) );
   if( obj ) {
      mutable_db.remove( *obj );
      return 1;
   }
   return 0;
}

int32_t apply_context::store_i64( Name scope, Name table, Name key, const char* value, uint32_t valuelen ) {
   require_scope( scope );

   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(
                                                            AccountName(scope), 
                                                            AccountName(code), 
                                                            AccountName(table), 
                                                            AccountName(key) ) );
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
         o.key   = key;
         o.value.insert( 0, value, valuelen );
      });
      return valuelen;
   }
}

int32_t apply_context::store_i128i128( Name scope, Name table, uint128_t primary, uint128_t secondary,
                                       const char* value, uint32_t valuelen ) {
   const auto* obj = db.find<key128x128_value_object,by_scope_primary>( boost::make_tuple(
                                                            AccountName(scope), 
                                                            AccountName(code), 
                                                            AccountName(table), 
                                                            primary, secondary ) );
   idump(( *((fc::uint128_t*)&primary)) );
   idump(( *((fc::uint128_t*)&secondary)) );
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
         o.primary_key = primary;
         o.secondary_key = secondary;
         o.value.insert( 0, value, valuelen );
      });
      return valuelen;
   }
}

} } // namespace eos::chain
