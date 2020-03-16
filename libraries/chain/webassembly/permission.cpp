#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void unpack_provided_keys( flat_set<public_key_type>& keys, const char* pubkeys_data, uint32_t pubkeys_size ) {
      keys.clear();
      if( pubkeys_size == 0 ) return;

      keys = fc::raw::unpack<flat_set<public_key_type>>( pubkeys_data, pubkeys_size );
   }

   void unpack_provided_permissions( flat_set<permission_level>& permissions, const char* perms_data, uint32_t perms_size ) {
      permissions.clear();
      if( perms_size == 0 ) return;

      permissions = fc::raw::unpack<flat_set<permission_level>>( perms_data, perms_size );
   }

   bool interface::check_transaction_authorization( legacy_array_ptr<char> trx_data,     uint32_t trx_size,
                                                    legacy_array_ptr<char> pubkeys_data, uint32_t pubkeys_size,
                                                    legacy_array_ptr<char> perms_data,   uint32_t perms_size ) {
      transaction trx = fc::raw::unpack<transaction>( trx_data, trx_size );

      flat_set<public_key_type> provided_keys;
      unpack_provided_keys( provided_keys, pubkeys_data, pubkeys_size );

      flat_set<permission_level> provided_permissions;
      unpack_provided_permissions( provided_permissions, perms_data, perms_size );

      try {
         ctx.control
                .get_authorization_manager()
                .check_authorization( trx.actions,
                                      provided_keys,
                                      provided_permissions,
                                      fc::seconds(trx.delay_sec),
                                      std::bind(&transaction_context::checktime, &ctx.trx_context),
                                      false
                                    );
         return true;
      } catch( const authorization_exception& e ) {}

      return false;
   }

   bool interface::check_permission_authorization( account_name account, permission_name permission,
                                                   legacy_array_ptr<char> pubkeys_data, uint32_t pubkeys_size,
                                                   legacy_array_ptr<char> perms_data,   uint32_t perms_size,
                                                   uint64_t delay_us ) {
      EOS_ASSERT( delay_us <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
                  action_validate_exception, "provided delay is too large" );

      flat_set<public_key_type> provided_keys;
      unpack_provided_keys( provided_keys, pubkeys_data, pubkeys_size );

      flat_set<permission_level> provided_permissions;
      unpack_provided_permissions( provided_permissions, perms_data, perms_size );

      try {
         ctx.control
                .get_authorization_manager()
                .check_authorization( account,
                                      permission,
                                      provided_keys,
                                      provided_permissions,
                                      fc::microseconds(delay_us),
                                      std::bind(&transaction_context::checktime, &ctx.trx_context),
                                      false
                                    );
         return true;
      } catch( const authorization_exception& e ) {}

      return false;
   }

   int64_t get_permission_last_used( account_name account, permission_name permission ) {
      const auto& am = ctx.control.get_authorization_manager();
      return am.get_permission_last_used( am.get_permission({account, permission}) ).time_since_epoch().count();
   };

   int64_t get_account_creation_time( account_name account ) {
      auto* acct = ctx.db.find<account_object, by_name>(account);
      EOS_ASSERT( acct != nullptr, action_validate_exception,
                  "account '${account}' does not exist", ("account", account) );
      return time_point(acct->creation_date).time_since_epoch().count();
   }
}}} // ns eosio::chain::webassembly
