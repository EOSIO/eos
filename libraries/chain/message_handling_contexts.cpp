#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/key_value_object.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void message_validate_context::require_authorization(const types::AccountName& account) {

   auto itr = boost::find_if(msg.authorization, [&account](const types::AccountPermission& ap) {
      return ap.account == account;
   });

   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Required authorization ${auth} not found", ("auth", account));

   used_authorizations[itr - msg.authorization.begin()] = true;
}

void message_validate_context::require_scope(const types::AccountName& account)const {
   auto itr = boost::find_if(trx.scope, [&account](const auto& scope) {
      return scope == account;
   });

   EOS_ASSERT( itr != trx.scope.end(), tx_missing_scope,
              "Required scope ${scope} not declared by transaction", ("scope",account) );
}

void message_validate_context::require_recipient(const types::AccountName& account)const {
   auto itr = boost::find_if(msg.recipients, [&account](const auto& recipient) {
      return recipient == account;
   });

   EOS_ASSERT( itr != msg.recipients.end(), tx_missing_recipient,
              "Required recipient ${recipient} not declared by message", ("recipient",account)("recipients",msg.recipients) );
}

bool message_validate_context::all_authorizations_used() const {
   return boost::algorithm::all_of_equal(used_authorizations, true);
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
      return 1;
   }
}

} } // namespace eos::chain
