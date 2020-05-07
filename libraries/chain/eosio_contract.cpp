#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/contract_table_objects.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_types.hpp>

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>

namespace eosio { namespace chain {



uint128_t transaction_id_to_sender_id( const transaction_id_type& tid ) {
   fc::uint128_t _id(tid._hash[3], tid._hash[2]);
   return (unsigned __int128)_id;
}

void validate_authority_precondition( const apply_context& context, const authority& auth ) {
   for(const auto& a : auth.accounts) {
      auto* acct = context.db.find<account_object, by_name>(a.permission.actor);
      EOS_ASSERT( acct != nullptr, action_validate_exception,
                  "account '${account}' does not exist",
                  ("account", a.permission.actor)
                );

      if( a.permission.permission == config::owner_name || a.permission.permission == config::active_name )
         continue; // account was already checked to exist, so its owner and active permissions should exist

      if( a.permission.permission == config::eosio_code_name ) // virtual eosio.code permission does not really exist but is allowed
         continue;

      try {
         context.control.get_authorization_manager().get_permission({a.permission.actor, a.permission.permission});
      } catch( const permission_query_exception& ) {
         EOS_THROW( action_validate_exception,
                    "permission '${perm}' does not exist",
                    ("perm", a.permission)
                  );
      }
   }

   if( context.trx_context.enforce_whiteblacklist && context.control.is_producing_block() ) {
      for( const auto& p : auth.keys ) {
         context.control.check_key_list( p.key );
      }
   }
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds a
 */
void apply_eosio_newaccount(apply_context& context) {
   auto create = context.get_action().data_as<newaccount>();
   try {
   context.require_authorization(create.creator);
//   context.require_write_lock( config::eosio_auth_scope );
   auto& authorization = context.control.get_mutable_authorization_manager();

   EOS_ASSERT( validate(create.owner), action_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), action_validate_exception, "Invalid active authority");

   auto& db = context.db;

   auto name_str = name(create.name).to_string();

   EOS_ASSERT( !create.name.empty(), action_validate_exception, "account name cannot be empty" );
   EOS_ASSERT( name_str.size() <= 12, action_validate_exception, "account names can only be 12 chars long" );

   // Check if the creator is privileged
   const auto &creator = db.get<account_metadata_object, by_name>(create.creator);
   if( !creator.is_privileged() ) {
      EOS_ASSERT( name_str.find( "eosio." ) != 0, action_validate_exception,
                  "only privileged accounts can have names that start with 'eosio.'" );
   }

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, account_name_exists_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   const auto& new_account = db.create<account_object>([&](auto& a) {
      a.name = create.name;
      a.creation_date = context.control.pending_block_time();
   });

   db.create<account_metadata_object>([&](auto& a) {
      a.name = create.name;
   });

   for( const auto& auth : { create.owner, create.active } ){
      validate_authority_precondition( context, auth );
   }

   const auto& owner_permission  = authorization.create_permission( create.name, config::owner_name, 0,
                                                                    std::move(create.owner), context.get_action_id() );
   const auto& active_permission = authorization.create_permission( create.name, config::active_name, owner_permission.id,
                                                                    std::move(create.active), context.get_action_id() );

   context.control.get_mutable_resource_limits_manager().initialize_account(create.name);

   int64_t ram_delta = config::overhead_per_account_ram_bytes;
   ram_delta += 2*config::billable_size_v<permission_object>;
   ram_delta += owner_permission.auth.get_billable_size();
   ram_delta += active_permission.auth.get_billable_size();

   std::string event_id;
   if (context.control.get_deep_mind_logger() != nullptr) {
      event_id = STORAGE_EVENT_ID("${name}", ("name", create.name));
   }

   context.add_ram_usage(create.name, ram_delta, storage_usage_trace(context.get_action_id(), event_id.c_str(), "account", "add", "newaccount"));

} FC_CAPTURE_AND_RETHROW( (create) ) }

void apply_eosio_setcode(apply_context& context) {
   const auto& cfg = context.control.get_global_properties().configuration;

   auto& db = context.db;
   auto  act = context.get_action().data_as<setcode>();
   context.require_authorization(act.account);

   EOS_ASSERT( act.vmtype == 0, invalid_contract_vm_type, "code should be 0" );
   EOS_ASSERT( act.vmversion == 0, invalid_contract_vm_version, "version should be 0" );

   fc::sha256 code_hash; /// default is the all zeros hash

   int64_t code_size = (int64_t)act.code.size();

   if( code_size > 0 ) {
     code_hash = fc::sha256::hash( act.code.data(), (uint32_t)act.code.size() );
     wasm_interface::validate(context.control, act.code);
   }

   const auto& account = db.get<account_metadata_object,by_name>(act.account);
   bool existing_code = (account.code_hash != digest_type());

   EOS_ASSERT( code_size > 0 || existing_code, set_exact_code, "contract is already cleared" );

   int64_t old_size  = 0;
   int64_t new_size  = code_size * config::setcode_ram_bytes_multiplier;

   if( existing_code ) {
      const code_object& old_code_entry = db.get<code_object, by_code_hash>(boost::make_tuple(account.code_hash, account.vm_type, account.vm_version));
      EOS_ASSERT( old_code_entry.code_hash != code_hash, set_exact_code,
                  "contract is already running this version of code" );
      old_size  = (int64_t)old_code_entry.code.size() * config::setcode_ram_bytes_multiplier;
      if( old_code_entry.code_ref_count == 1 ) {
         db.remove(old_code_entry);
         context.control.get_wasm_interface().code_block_num_last_used(account.code_hash, account.vm_type, account.vm_version, context.control.head_block_num() + 1);
      } else {
         db.modify(old_code_entry, [](code_object& o) {
            --o.code_ref_count;
         });
      }
   }

   if( code_size > 0 ) {
      const code_object* new_code_entry = db.find<code_object, by_code_hash>(
                                             boost::make_tuple(code_hash, act.vmtype, act.vmversion) );
      if( new_code_entry ) {
         db.modify(*new_code_entry, [&](code_object& o) {
            ++o.code_ref_count;
         });
      } else {
         db.create<code_object>([&](code_object& o) {
            o.code_hash = code_hash;
            o.code.assign(act.code.data(), code_size);
            o.code_ref_count = 1;
            o.first_block_used = context.control.head_block_num() + 1;
            o.vm_type = act.vmtype;
            o.vm_version = act.vmversion;
         });
      }
   }

   db.modify( account, [&]( auto& a ) {
      a.code_sequence += 1;
      a.code_hash = code_hash;
      a.vm_type = act.vmtype;
      a.vm_version = act.vmversion;
      a.last_code_update = context.control.pending_block_time();
   });

   if (new_size != old_size) {
      std::string operation;
      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         operation = "update";
         if (old_size <= 0) {
            operation = "add";
         } else if (new_size <= 0) {
            operation = "remove";
         }

         event_id = STORAGE_EVENT_ID("${account}", ("account", act.account));
      }

      context.add_ram_usage( act.account, new_size - old_size, storage_usage_trace(context.get_action_id(), event_id.c_str(), "code", operation.c_str(), "setcode") );
   }
}

void apply_eosio_setabi(apply_context& context) {
   auto& db  = context.db;
   auto  act = context.get_action().data_as<setabi>();

   context.require_authorization(act.account);

   const auto& account = db.get<account_object,by_name>(act.account);

   int64_t abi_size = act.abi.size();

   int64_t old_size = (int64_t)account.abi.size();
   int64_t new_size = abi_size;

   db.modify( account, [&]( auto& a ) {
      a.abi.assign(act.abi.data(), abi_size);
   });

   const auto& account_metadata = db.get<account_metadata_object, by_name>(act.account);
   db.modify( account_metadata, [&]( auto& a ) {
      a.abi_sequence += 1;
   });

   if (new_size != old_size) {
      std::string operation;
      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         operation = "update";
         if (old_size <= 0) {
            operation = "add";
         } else if (new_size <= 0) {
            operation = "remove";
         }

         event_id = STORAGE_EVENT_ID("${account}", ("account", act.account));
      }

      context.add_ram_usage( act.account, new_size - old_size, storage_usage_trace(context.get_action_id(), event_id.c_str(), "abi", operation.c_str(), "setabi") );
   }
}

void apply_eosio_updateauth(apply_context& context) {

   auto update = context.get_action().data_as<updateauth>();
   context.require_authorization(update.account); // only here to mark the single authority on this action as used

   auto& authorization = context.control.get_mutable_authorization_manager();
   auto& db = context.db;

   EOS_ASSERT(!update.permission.empty(), action_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT( update.permission.to_string().find( "eosio." ) != 0, action_validate_exception,
               "Permission names that start with 'eosio.' are reserved" );
   EOS_ASSERT(update.permission != update.parent, action_validate_exception, "Cannot set an authority as its own parent");
   db.get<account_object, by_name>(update.account);
   EOS_ASSERT(validate(update.auth), action_validate_exception,
              "Invalid authority: ${auth}", ("auth", update.auth));
   if( update.permission == config::active_name )
      EOS_ASSERT(update.parent == config::owner_name, action_validate_exception, "Cannot change active authority's parent from owner", ("update.parent", update.parent) );
   if (update.permission == config::owner_name)
      EOS_ASSERT(update.parent.empty(), action_validate_exception, "Cannot change owner authority's parent");
   else
      EOS_ASSERT(!update.parent.empty(), action_validate_exception, "Only owner permission can have empty parent" );

   if( update.auth.waits.size() > 0 ) {
      auto max_delay = context.control.get_global_properties().configuration.max_transaction_delay;
      EOS_ASSERT( update.auth.waits.back().wait_sec <= max_delay, action_validate_exception,
                  "Cannot set delay longer than max_transacton_delay, which is ${max_delay} seconds",
                  ("max_delay", max_delay) );
   }

   validate_authority_precondition(context, update.auth);



   auto permission = authorization.find_permission({update.account, update.permission});

   // If a parent_id of 0 is going to be used to indicate the absence of a parent, then we need to make sure that the chain
   // initializes permission_index with a dummy object that reserves the id of 0.
   authorization_manager::permission_id_type parent_id = 0;
   if( update.permission != config::owner_name ) {
      auto& parent = authorization.get_permission({update.account, update.parent});
      parent_id = parent.id;
   }

   if( permission ) {
      EOS_ASSERT(parent_id == permission->parent, action_validate_exception,
                 "Changing parent authority is not currently supported");


      int64_t old_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());

      authorization.modify_permission( *permission, update.auth, context.get_action_id() );

      int64_t new_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = STORAGE_EVENT_ID("${id}", ("id", permission->id));
      }

      context.add_ram_usage( permission->owner, new_size - old_size, storage_usage_trace(context.get_action_id(), event_id.c_str(), "auth", "update", "updateauth_update") );
   } else {
      const auto& p = authorization.create_permission( update.account, update.permission, parent_id, update.auth, context.get_action_id() );

      int64_t new_size = (int64_t)(config::billable_size_v<permission_object> + p.auth.get_billable_size());

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = STORAGE_EVENT_ID("${id}", ("id", p.id));
      }

      context.add_ram_usage( update.account, new_size, storage_usage_trace(context.get_action_id(), event_id.c_str(), "auth", "add", "updateauth_create") );
   }
}

void apply_eosio_deleteauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto remove = context.get_action().data_as<deleteauth>();
   context.require_authorization(remove.account); // only here to mark the single authority on this action as used

   EOS_ASSERT(remove.permission != config::active_name, action_validate_exception, "Cannot delete active authority");
   EOS_ASSERT(remove.permission != config::owner_name, action_validate_exception, "Cannot delete owner authority");

   auto& authorization = context.control.get_mutable_authorization_manager();
   auto& db = context.db;



   { // Check for links to this permission
      const auto& index = db.get_index<permission_link_index, by_permission_name>();
      auto range = index.equal_range(boost::make_tuple(remove.account, remove.permission));
      EOS_ASSERT(range.first == range.second, action_validate_exception,
                 "Cannot delete a linked authority. Unlink the authority first. This authority is linked to ${code}::${type}.",
                 ("code", range.first->code)("type", range.first->message_type));
   }

   const auto& permission = authorization.get_permission({remove.account, remove.permission});
   int64_t old_size = config::billable_size_v<permission_object> + permission.auth.get_billable_size();

   std::string event_id;
   if (context.control.get_deep_mind_logger() != nullptr) {
      event_id = STORAGE_EVENT_ID("${id}", ("id", permission.id));
   }

   authorization.remove_permission( permission, context.get_action_id() );

   context.add_ram_usage( remove.account, -old_size, storage_usage_trace(context.get_action_id(), event_id.c_str(), "auth", "remove", "deleteauth") );
}

void apply_eosio_linkauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto requirement = context.get_action().data_as<linkauth>();
   try {
      EOS_ASSERT(!requirement.requirement.empty(), action_validate_exception, "Required permission cannot be empty");

      context.require_authorization(requirement.account); // only here to mark the single authority on this action as used

      auto& db = context.db;
      const auto *account = db.find<account_object, by_name>(requirement.account);
      EOS_ASSERT(account != nullptr, account_query_exception,
                 "Failed to retrieve account: ${account}", ("account", requirement.account)); // Redundant?
      const auto *code = db.find<account_object, by_name>(requirement.code);
      EOS_ASSERT(code != nullptr, account_query_exception,
                 "Failed to retrieve code for account: ${account}", ("account", requirement.code));
      if( requirement.requirement != config::eosio_any_name ) {
         const permission_object* permission = nullptr;
         if( context.control.is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) ) {
            permission = db.find<permission_object, by_owner>(
                           boost::make_tuple( requirement.account, requirement.requirement )
                         );
         } else {
            permission = db.find<permission_object, by_name>(requirement.requirement);
         }

         EOS_ASSERT(permission != nullptr, permission_query_exception,
                    "Failed to retrieve permission: ${permission}", ("permission", requirement.requirement));
      }

      auto link_key = boost::make_tuple(requirement.account, requirement.code, requirement.type);
      auto link = db.find<permission_link_object, by_action_name>(link_key);

      if( link ) {
         EOS_ASSERT(link->required_permission != requirement.requirement, action_validate_exception,
                    "Attempting to update required authority, but new requirement is same as old");
         db.modify(*link, [requirement = requirement.requirement](permission_link_object& link) {
             link.required_permission = requirement;
         });
      } else {
         const auto& l =  db.create<permission_link_object>([&requirement](permission_link_object& link) {
            link.account = requirement.account;
            link.code = requirement.code;
            link.message_type = requirement.type;
            link.required_permission = requirement.requirement;
         });

         std::string event_id;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = STORAGE_EVENT_ID("${id}", ("id", l.id));
         }

         context.add_ram_usage(
            l.account,
            (int64_t)(config::billable_size_v<permission_link_object>),
            storage_usage_trace(context.get_action_id(), event_id.c_str(), "auth_link", "add", "linkauth")
         );
      }

  } FC_CAPTURE_AND_RETHROW((requirement))
}

void apply_eosio_unlinkauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto& db = context.db;
   auto unlink = context.get_action().data_as<unlinkauth>();

   context.require_authorization(unlink.account); // only here to mark the single authority on this action as used

   auto link_key = boost::make_tuple(unlink.account, unlink.code, unlink.type);
   auto link = db.find<permission_link_object, by_action_name>(link_key);
   EOS_ASSERT(link != nullptr, action_validate_exception, "Attempting to unlink authority, but no link found");

   std::string event_id;
   if (context.control.get_deep_mind_logger() != nullptr) {
      event_id = STORAGE_EVENT_ID("${id}", ("id", link->id));
   }

   context.add_ram_usage(
      link->account,
      -(int64_t)(config::billable_size_v<permission_link_object>),
      storage_usage_trace(context.get_action_id(), event_id.c_str(), "auth_link", "remove", "unlinkauth")
   );

   db.remove(*link);
}

void apply_eosio_canceldelay(apply_context& context) {
   auto cancel = context.get_action().data_as<canceldelay>();
   context.require_authorization(cancel.canceling_auth.actor); // only here to mark the single authority on this action as used

   const auto& trx_id = cancel.trx_id;

   context.cancel_deferred_transaction(transaction_id_to_sender_id(trx_id), account_name());
}

} } // namespace eosio::chain
