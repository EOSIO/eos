/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/eos_contract.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/contracts/chain_initializer.hpp>

#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/producer_object.hpp>

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <eosio/chain/resource_limits.hpp>

namespace eosio { namespace chain { namespace contracts {

void validate_authority_precondition( const apply_context& context, const authority& auth ) {
   for(const auto& a : auth.accounts) {
      context.db.get<account_object, by_name>(a.permission.actor);
      context.db.get<permission_object, by_owner>(boost::make_tuple(a.permission.actor, a.permission.permission));
   }
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds a
 */
void apply_eosio_newaccount(apply_context& context) {
   auto create = context.act.data_as<newaccount>();
   try {
   context.require_authorization(create.creator);
   context.require_write_lock( config::eosio_auth_scope );
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();

   EOS_ASSERT( validate(create.owner), action_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), action_validate_exception, "Invalid active authority");
   EOS_ASSERT( validate(create.recovery), action_validate_exception, "Invalid recovery authority");

   auto& db = context.mutable_db;

   auto name_str = name(create.name).to_string();

   EOS_ASSERT( !create.name.empty(), action_validate_exception, "account name cannot be empty" );
   EOS_ASSERT( name_str.size() <= 12, action_validate_exception, "account names can only be 12 chars long" );

   // Check if the creator is privileged
   const auto &creator = db.get<account_object, by_name>(create.creator);
   if( !creator.privileged ) {
      EOS_ASSERT( name_str.find( "eosio." ) != 0, action_validate_exception,
                  "only privileged accounts can have names that start with 'eosio.'" );
   }

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, action_validate_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   for( const auto& auth : { create.owner, create.active, create.recovery } ){
      validate_authority_precondition( context, auth );
   }

   const auto& new_account = db.create<account_object>([&create, &context](account_object& a) {
      a.name = create.name;
      a.creation_date = context.controller.head_block_time();
   });
   resources.initialize_account(create.name);
   resources.add_pending_account_ram_usage(
      create.creator,
      (int64_t)config::overhead_per_account_ram_bytes
   );

   auto create_permission = [owner=create.name, &db, &context, &resources](const permission_name& name, permission_object::id_type parent, authority &&auth) {
      const auto& result = db.create<permission_object>([&](permission_object& p) {
         p.name = name;
         p.parent = parent;
         p.owner = owner;
         p.auth = std::move(auth);
      });

      resources.add_pending_account_ram_usage(
         owner,
         (int64_t)(config::billable_size_v<permission_object> + result.auth.get_billable_size())
      );

      return result;
   };

   // If a parent_id of 0 is going to be used to indicate the absence of a parent, then we need to make sure that the chain
   // initializes permission_index with a dummy object that reserves the id of 0.
   const auto& owner_permission = create_permission(config::owner_name, 0, std::move(create.owner));
   create_permission(config::active_name, owner_permission.id, std::move(create.active));

} FC_CAPTURE_AND_RETHROW( (create) ) }

void apply_eosio_setcode(apply_context& context) {
   auto& db = context.mutable_db;
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   auto  act = context.act.data_as<setcode>();
   context.require_authorization(act.account);
   context.require_write_lock( config::eosio_auth_scope );

   FC_ASSERT( act.vmtype == 0 );
   FC_ASSERT( act.vmversion == 0 );

   auto code_id = fc::sha256::hash( act.code.data(), (uint32_t)act.code.size() );

   wasm_interface::validate(act.code);

   const auto& account = db.get<account_object,by_name>(act.account);

   int64_t code_size = (int64_t)act.code.size();
   int64_t old_size = (int64_t)account.code.size() * config::setcode_ram_bytes_multiplier;
   int64_t new_size = code_size * config::setcode_ram_bytes_multiplier;


   FC_ASSERT( account.code_version != code_id, "contract is already running this version of code" );
//   wlog( "set code: ${size}", ("size",act.code.size()));
   db.modify( account, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient to detect code version changes*/
      #warning TODO: update setcode message to include the hash, then validate it in validate
      a.code_version = code_id;
      // Added resize(0) here to avoid bug in boost vector container
      a.code.resize( 0 );
      a.code.resize( code_size );
      a.last_code_update = context.controller.head_block_time();
      memcpy( a.code.data(), act.code.data(), code_size );

   });

   if (new_size != old_size) {
      resources.add_pending_account_ram_usage(
         act.account,
         new_size - old_size
      );
   }
}

void apply_eosio_setabi(apply_context& context) {
   auto& db = context.mutable_db;
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   auto  act = context.act.data_as<setabi>();

   context.require_authorization(act.account);

   // if system account append native abi
   if ( act.account == eosio::chain::config::system_account_name ) {
      act.abi = chain_initializer::eos_contract_abi(act.abi);
   }
   /// if an ABI is specified make sure it is well formed and doesn't
   /// reference any undefined types
   abi_serializer(act.abi).validate();
   // todo: figure out abi serialization location

   const auto& account = db.get<account_object,by_name>(act.account);

   int64_t old_size = (int64_t)account.abi.size();
   int64_t new_size = (int64_t)fc::raw::pack_size(act.abi);

   db.modify( account, [&]( auto& a ) {
      a.set_abi( act.abi );
   });

   if (new_size != old_size) {
      resources.add_pending_account_ram_usage(
         act.account,
         new_size - old_size
      );
   }
}

void apply_eosio_updateauth(apply_context& context) {
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   context.require_write_lock( config::eosio_auth_scope );

   auto& db = context.mutable_db;

   auto update = context.act.data_as<updateauth>();
   EOS_ASSERT(!update.permission.empty(), action_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT( update.permission.to_string().find( "eosio." ) != 0, action_validate_exception,
               "Permission names that start with 'eosio.' are reserved" );
   EOS_ASSERT(update.permission != update.parent, action_validate_exception, "Cannot set an authority as its own parent");
   db.get<account_object, by_name>(update.account);
   EOS_ASSERT(validate(update.data), action_validate_exception,
              "Invalid authority: ${auth}", ("auth", update.data));
   if( update.permission == config::active_name )
      EOS_ASSERT(update.parent == config::owner_name, action_validate_exception, "Cannot change active authority's parent from owner", ("update.parent", update.parent) );
   if (update.permission == config::owner_name)
      EOS_ASSERT(update.parent.empty(), action_validate_exception, "Cannot change owner authority's parent");
   else
      EOS_ASSERT(!update.parent.empty(), action_validate_exception, "Only owner permission can have empty parent" );

   auto max_delay = context.controller.get_global_properties().configuration.max_transaction_delay;
   EOS_ASSERT( update.delay <= max_delay, action_validate_exception, "Cannot set delay longer than max_transacton_delay, which is ${max_delay} seconds", ("max_delay", max_delay) );

   FC_ASSERT(context.act.authorization.size(), "updateauth can only have one action authorization");
   const auto& act_auth = context.act.authorization.front();
   // lazy evaluating loop
   auto permission_is_valid_for_update = [&](){
      if (act_auth.permission == config::owner_name || act_auth.permission == update.permission) {
         return true;
      }
      const permission_object *current = db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));
      // Permission doesn't exist yet, check parent permission
      if (current == nullptr) current = db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.parent));
      // Ensure either the permission or parent's permission exists
      EOS_ASSERT(current != nullptr, permission_query_exception,
                 "Failed to retrieve permission for: {\"actor\": \"${actor}\", \"permission\": \"${permission}\" }",
                 ("actor", update.account)("permission", update.parent));

      while(current->name != config::owner_name) {
         if (current->name == act_auth.permission) {
            return true;
         }
         current = &db.get<permission_object>(current->parent);
      }

      return false;
   };

   FC_ASSERT(act_auth.actor == update.account && permission_is_valid_for_update(), "updateauth must carry a permission equal to or in the ancestery of permission it updates");

   validate_authority_precondition(context, update.data);

   auto permission = db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));

   // If a parent_id of 0 is going to be used to indicate the absence of a parent, then we need to make sure that the chain
   // initializes permission_index with a dummy object that reserves the id of 0.
   permission_object::id_type parent_id = 0;
   if(update.permission != config::owner_name) {
      auto& parent = db.get<permission_object, by_owner>(boost::make_tuple(update.account, update.parent));
      parent_id = parent.id;
   }

   if (permission) {
      EOS_ASSERT(parent_id == permission->parent, action_validate_exception,
                 "Changing parent authority is not currently supported");


      int64_t old_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());

      // TODO/QUESTION: If we are updating an existing permission, should we check if the message declared
      // permission satisfies the permission we want to modify?
      db.modify(*permission, [&update, &parent_id, &context](permission_object& po) {
         po.auth = update.data;
         po.parent = parent_id;
         po.last_updated = context.controller.head_block_time();
         po.delay = fc::seconds(update.delay);
      });

      int64_t new_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());

      resources.add_pending_account_ram_usage(
         permission->owner,
         new_size - old_size
      );
   } else {
      // TODO/QUESTION: If we are creating a new permission, should we check if the message declared
      // permission satisfies the parent permission?
      const auto& p = db.create<permission_object>([&update, &parent_id, &context](permission_object& po) {
         po.name = update.permission;
         po.owner = update.account;
         po.auth = update.data;
         po.parent = parent_id;
         po.last_updated = context.controller.head_block_time();
         po.delay = fc::seconds(update.delay);
      });

      resources.add_pending_account_ram_usage(
         p.owner,
         (int64_t)(config::billable_size_v<permission_object> + p.auth.get_billable_size())
      );

   }
}

void apply_eosio_deleteauth(apply_context& context) {
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   auto remove = context.act.data_as<deleteauth>();
   EOS_ASSERT(remove.permission != config::active_name, action_validate_exception, "Cannot delete active authority");
   EOS_ASSERT(remove.permission != config::owner_name, action_validate_exception, "Cannot delete owner authority");

   auto& db = context.mutable_db;
   context.require_authorization(remove.account);
   // TODO/QUESTION:
   //   Inconsistency between permissions that can be satisfied to create/modify (via updateauth) a permission and the
   //   stricter requirements for deleting the permission using deleteauth.
   //   If a permission can be updated, shouldn't it also be allowed to delete it without higher permissions required?
   const auto& permission = db.get<permission_object, by_owner>(boost::make_tuple(remove.account, remove.permission));

   { // Check for children
      const auto& index = db.get_index<permission_index, by_parent>();
      auto range = index.equal_range(permission.id);
      EOS_ASSERT(range.first == range.second, action_validate_exception,
                 "Cannot delete an authority which has children. Delete the children first");
   }

   { // Check for links to this permission
      const auto& index = db.get_index<permission_link_index, by_permission_name>();
      auto range = index.equal_range(boost::make_tuple(remove.account, remove.permission));
      EOS_ASSERT(range.first == range.second, action_validate_exception,
                 "Cannot delete a linked authority. Unlink the authority first");
   }

   resources.add_pending_account_ram_usage(
      permission.owner,
      -(int64_t)(config::billable_size_v<permission_object> + permission.auth.get_billable_size())
   );
   db.remove(permission);
}

void apply_eosio_linkauth(apply_context& context) {
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   auto requirement = context.act.data_as<linkauth>();
   try {
      EOS_ASSERT(!requirement.requirement.empty(), action_validate_exception, "Required permission cannot be empty");

      context.require_authorization(requirement.account);

      auto& db = context.mutable_db;
      const auto *account = db.find<account_object, by_name>(requirement.account);
      EOS_ASSERT(account != nullptr, account_query_exception,
                 "Failed to retrieve account: ${account}", ("account", requirement.account)); // Redundant?
      const auto *code = db.find<account_object, by_name>(requirement.code);
      EOS_ASSERT(code != nullptr, account_query_exception,
                 "Failed to retrieve code for account: ${account}", ("account", requirement.code));
      if( requirement.requirement != config::eosio_any_name ) {
         const auto *permission = db.find<permission_object, by_name>(requirement.requirement);
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

         resources.add_pending_account_ram_usage(
            l.account,
            (int64_t)(config::billable_size_v<permission_link_object>)
         );
      }
  } FC_CAPTURE_AND_RETHROW((requirement))
}

void apply_eosio_unlinkauth(apply_context& context) {
   auto& resources = context.mutable_controller.get_mutable_resource_limits_manager();
   auto& db = context.mutable_db;
   auto unlink = context.act.data_as<unlinkauth>();

   context.require_authorization(unlink.account);

   auto link_key = boost::make_tuple(unlink.account, unlink.code, unlink.type);
   auto link = db.find<permission_link_object, by_action_name>(link_key);
   EOS_ASSERT(link != nullptr, action_validate_exception, "Attempting to unlink authority, but no link found");
   resources.add_pending_account_ram_usage(
      link->account,
      -(int64_t)(config::billable_size_v<permission_link_object>)
   );

   db.remove(*link);
}


void apply_eosio_onerror(apply_context& context) {
   FC_ASSERT(context.trx_meta.sender.valid(), "onerror action cannot be called directly");
   context.require_recipient(*context.trx_meta.sender);
}

static const abi_serializer& get_abi_serializer() {
   static optional<abi_serializer> _abi_serializer;
   if (!_abi_serializer) {
      _abi_serializer.emplace(chain_initializer::eos_contract_abi(abi_def()));
   }

   return *_abi_serializer;
}

static optional<variant> get_pending_recovery(apply_context& context, account_name account ) {
   const uint64_t id = account;
   const auto table = N(recovery);
   const auto iter = context.db_find_i64(config::system_account_name, account, table, id);
   if (iter != -1) {
      const auto buffer_size = context.db_get_i64(iter, nullptr, 0);
      bytes value(buffer_size);

      const auto written_size = context.db_get_i64(iter, value.data(), buffer_size);
      assert(written_size == buffer_size);

      return get_abi_serializer().binary_to_variant("pending_recovery", value);
   }

   return optional<variant_object>();
}

static auto get_account_creation(const apply_context& context, const account_name& account) {
   auto const& accnt = context.db.get<account_object, by_name>(account);
   return (time_point)accnt.creation_date;
};

static auto get_permission_last_used(const apply_context& context, const account_name& account, const permission_name& permission) {
   auto const* perm = context.db.find<permission_usage_object, by_account_permission>(boost::make_tuple(account, permission));
   if (perm) {
      return optional<time_point>(perm->last_used);
   }

   return optional<time_point>();
};

void apply_eosio_postrecovery(apply_context& context) {
   context.require_write_lock( config::eosio_auth_scope );

   FC_ASSERT(context.act.authorization.size() == 1, "Recovery Message must have exactly one authorization");

   auto recover_act = context.act.data_as<postrecovery>();
   const auto &auth = context.act.authorization.front();
   const auto& account = recover_act.account;
   context.require_write_lock(account);

   FC_ASSERT(!get_pending_recovery(context, account), "Account ${account} already has a pending recovery request!", ("account",account));

   fc::microseconds delay_lock;
   auto now = context.controller.head_block_time();
   if (auth.actor == account && auth.permission == N(active)) {
      // process owner recovery from active
      delay_lock = fc::days(30);
   } else {
      // process lost password

      auto owner_last_used = get_permission_last_used(context, account, N(owner));
      auto active_last_used = get_permission_last_used(context, account, N(active));

      if (!owner_last_used || !active_last_used) {
         auto account_creation = get_account_creation(context, account);
         if (!owner_last_used) {
            owner_last_used.emplace(account_creation);
         }

         if (!active_last_used) {
            active_last_used.emplace(account_creation);
         }
      }

      FC_ASSERT(*owner_last_used <= now - fc::days(30), "Account ${account} has had owner key activity recently and cannot be recovered yet!", ("account",account));
      FC_ASSERT(*active_last_used <= now - fc::days(30), "Account ${account} has had active key activity recently and cannot be recovered yet!", ("account",account));

      delay_lock = fc::days(7);
   }

   variant update;
   fc::to_variant( updateauth {
      .account = account,
      .permission = N(owner),
      .parent = 0,
      .data = recover_act.data
   }, update);

   const uint128_t request_id = context.controller.transaction_id_to_sender_id(context.trx_meta.id);
   auto record_data = mutable_variant_object()
      ("account", account)
      ("request_id", request_id)
      ("update", update)
      ("memo", recover_act.memo);

   deferred_transaction dtrx;
   dtrx.sender    = config::system_account_name;
   dtrx.sender_id = request_id;
   dtrx.payer     = config::system_account_name; // NOTE: we pre-reserve capacity for this during create account
   dtrx.region    = 0;
   dtrx.execute_after = context.controller.head_block_time() + delay_lock;
   dtrx.set_reference_block(context.controller.head_block_id());
   dtrx.expiration = dtrx.execute_after + fc::seconds(60);
   dtrx.actions.emplace_back(vector<permission_level>{{account,config::active_name}},
                             passrecovery { account });

   context.execute_deferred(std::move(dtrx));

   auto data = get_abi_serializer().variant_to_binary("pending_recovery", record_data);
   const uint64_t id = account;
   const uint64_t table = N(recovery);
   const auto payer = account;

   const auto iter = context.db_find_i64(config::system_account_name, account, table, id);
   if (iter == -1) {
      context.db_store_i64(account, table, payer, id, (const char*)data.data(), data.size());
   } else {
      context.db_update_i64(iter, payer, (const char*)data.data(), data.size());
   }

   context.console_append_formatted("Recovery Started for account ${account} : ${memo}\n", mutable_variant_object()("account", account)("memo", recover_act.memo));
}

static void remove_pending_recovery(apply_context& context, const account_name& account) {
   const auto iter = context.db_find_i64(config::system_account_name, account, N(recovery), account);
   if (iter != -1) {
      context.db_remove_i64(iter);
   }
}

void apply_eosio_passrecovery(apply_context& context) {
   auto pass_act = context.act.data_as<passrecovery>();
   const auto& account = pass_act.account;

   // ensure this is only processed if it is a deferred transaction from the system account
   FC_ASSERT(context.trx_meta.sender && *context.trx_meta.sender == config::system_account_name);
   context.require_authorization(account);

   auto maybe_recovery = get_pending_recovery(context, account);
   FC_ASSERT(maybe_recovery, "No pending recovery found for account ${account}", ("account", account));
   auto recovery = *maybe_recovery;

   updateauth update;
   fc::from_variant(recovery["update"], update);

   action act(vector<permission_level>{{account,config::owner_name}}, update);
   context.execute_inline(move(act));

   remove_pending_recovery(context, account);
   context.console_append_formatted("Account ${account} successfully recovered!\n", mutable_variant_object()("account", account));
}

void apply_eosio_vetorecovery(apply_context& context) {
   context.require_write_lock( config::eosio_auth_scope );
   auto pass_act = context.act.data_as<vetorecovery>();
   const auto& account = pass_act.account;
   context.require_authorization(account);

   auto maybe_recovery = get_pending_recovery(context, account);
   FC_ASSERT(maybe_recovery, "No pending recovery found for account ${account}", ("account", account));
   auto recovery = *maybe_recovery;

   context.cancel_deferred(recovery["request_id"].as<uint128_t>());

   remove_pending_recovery(context, account);
   context.console_append_formatted("Recovery for account ${account} vetoed!\n", mutable_variant_object()("account", account));
}

void apply_eosio_canceldelay(apply_context& context) {
   auto cancel = context.act.data_as<canceldelay>();
   const auto& trx_id = cancel.trx_id;

   const auto& generated_transaction_idx = context.controller.get_database().get_index<generated_transaction_multi_index>();
   const auto& generated_index = generated_transaction_idx.indices().get<by_trx_id>();
   const auto& itr = generated_index.lower_bound(trx_id);
   FC_ASSERT (itr != generated_index.end() && itr->sender == config::system_account_name && itr->trx_id == trx_id,
              "cannot cancel trx_id=${tid}, there is no deferred transaction with that transaction id",("tid", trx_id));

   auto dtrx = fc::raw::unpack<deferred_transaction>(itr->packed_trx.data(), itr->packed_trx.size());
   set<account_name> accounts;
   for (const auto& act : dtrx.actions) {
      for (const auto& auth : act.authorization) {
         accounts.insert(auth.actor);
      }
   }

   bool found = false;
   for (const auto& auth : context.act.authorization) {
      if (auth.permission == config::active_name && accounts.count(auth.actor)) {
         found = true;
         break;
      }
   }

   FC_ASSERT (found, "canceldelay action must be signed with the \"active\" permission for one of the actors"
                     " provided in the authorizations on the original transaction");

   context.cancel_deferred(context.controller.transaction_id_to_sender_id(trx_id));
}

} } } // namespace eosio::chain::contracts
