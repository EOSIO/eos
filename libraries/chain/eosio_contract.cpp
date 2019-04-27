/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/system_contracts.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/producer_object.hpp>

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <cyberway/chaindb/controller.hpp>

namespace eosio { namespace chain {



uint128_t transaction_id_to_sender_id( const transaction_id_type& tid ) {
   fc::uint128_t _id(tid._hash[3], tid._hash[2]);
   return (unsigned __int128)_id;
}

void validate_authority_precondition( const apply_context& context, const authority& auth ) {
   for(const auto& a : auth.accounts) {
      auto* acct = context.chaindb.find<account_object, by_name>(a.permission.actor);
      EOS_ASSERT( acct != nullptr, action_validate_exception,
                  "account '${account}' does not exist",
                  ("account", a.permission.actor)
                );

      if( a.permission.permission == config::owner_name || a.permission.permission == config::active_name )
         continue; // account was already checked to exist, so its owner and active permissions should exist

      if( a.permission.permission == config::eosio_code_name ) // virtual cyber.code permission does not really exist but is allowed
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

   if( context.control.is_producing_block() ) {
      for( const auto& p : auth.keys ) {
         context.control.check_key_list( p.key );
      }
   }
}

const string& system_prefix() {
   static const string prefix = name{config::system_account_name}.to_string() + '.';
   return prefix;
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds a
 */
void apply_cyber_newaccount(apply_context& context) {
   auto create = context.act.data_as<newaccount>();
   try {
   context.require_authorization(create.creator);
//   context.require_write_lock( config::eosio_auth_scope );
   auto& authorization = context.control.get_mutable_authorization_manager();

   EOS_ASSERT( validate(create.owner), action_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), action_validate_exception, "Invalid active authority");

   auto& chaindb = context.chaindb;

   auto name_str = name(create.name).to_string();

   EOS_ASSERT( !create.name.empty(), action_validate_exception, "account name cannot be empty" );
   EOS_ASSERT( name_str.size() <= 12, action_validate_exception, "account names can only be 12 chars long" );

   // Check if the creator is privileged
   const auto &creator = chaindb.get<account_object, by_name>(create.creator);
   if( !creator.privileged ) {
      EOS_ASSERT(name_str.find(system_prefix()) != 0, action_validate_exception,
         "only privileged accounts can have names that start with '${prefix}'", ("prefix", system_prefix()));
   }

   auto existing_account = chaindb.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, account_name_exists_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   // CyberWay: set RAM usage to creator, it can rewrite memory usage to account, so no troubles
   auto ram_payer = context.get_storage_payer(create.name, create.creator);

   context.control.get_mutable_resource_limits_manager().initialize_account(create.name, ram_payer);

   chaindb.emplace<account_object>(ram_payer, [&](auto& a) {
      a.name = create.name;
      a.creation_date = context.control.pending_block_time();
   });

   chaindb.emplace<account_sequence_object>(ram_payer, [&](auto& a) {
      a.name = create.name;
   });

   for( const auto& auth : { create.owner, create.active } ){
      validate_authority_precondition( context, auth );
   }

   const auto& owner_permission  = authorization.create_permission( ram_payer,
                                                                    create.name, config::owner_name, 0,
                                                                    std::move(create.owner) );
   const auto& active_permission = authorization.create_permission( ram_payer,
                                                                    create.name, config::active_name, owner_permission.id,
                                                                    std::move(create.active) );

// TODO: Removed by CyberWay
//   int64_t ram_delta = config::overhead_per_account_ram_bytes;
//   ram_delta += 2*config::billable_size_v<permission_object>;
//   ram_delta += owner_permission.auth.get_billable_size();
//   ram_delta += active_permission.auth.get_billable_size();
//
//   context.add_ram_usage(create.name, ram_delta);

} FC_CAPTURE_AND_RETHROW( (create) ) }

// system account and accounts with system prefix (cyber.*) are protected
bool is_protected_account(name acc) {
    // TODO: prefix can be checked without string using binary AND
    return acc == config::system_account_name || name{acc}.to_string().find(system_prefix()) == 0;
}

fc::sha256 bytes_hash(bytes data) {
    fc::sha256 h;
    if (data.size() > 0) {
        h = fc::sha256::hash(data.data(), data.size());
    }
    return h;
}

bool check_hash_for_accseq(const digest_type& hash, const contract_hashes_map& map, name account, uint64_t sequence) {
    auto hashes = map.find(account);
    bool valid =
        hashes != map.end() &&
        sequence < hashes->second.size() &&
        hash == hashes->second[sequence];
    return valid;
}

#define ALLOW_INITIAL_CONTRACT_SET  // TODO: check testnet/mainnet?

void apply_cyber_setcode(apply_context& context) {
   const auto& cfg = context.control.get_global_properties().configuration;

   auto& chaindb = context.chaindb;
   auto  act = context.act.data_as<setcode>();
   context.require_authorization(act.account);

   EOS_ASSERT( act.vmtype == 0, invalid_contract_vm_type, "code should be 0" );
   EOS_ASSERT( act.vmversion == 0, invalid_contract_vm_version, "version should be 0" );

   fc::sha256 code_id; /// default ID == 0

   if( act.code.size() > 0 ) {
     code_id = fc::sha256::hash( act.code.data(), (uint32_t)act.code.size() );
     wasm_interface::validate(context.control, act.code);
   }

   const auto& account = chaindb.get<account_object,by_name>(act.account);

   int64_t code_size = (int64_t)act.code.size();
   int64_t old_size  = (int64_t)account.code.size() * config::setcode_ram_bytes_multiplier;
   int64_t new_size  = code_size * config::setcode_ram_bytes_multiplier;

   EOS_ASSERT( account.code_version != code_id, set_exact_code, "contract is already running this version of code" );
    const auto& account_sequence = chaindb.get<account_sequence_object, by_name>(act.account);
    if (is_protected_account(act.account)) {
        const auto seq = account_sequence.code_sequence;
        bool allowed = false;
#ifdef ALLOW_INITIAL_CONTRACT_SET
        allowed = seq == 0 && account.code_version == fc::sha256();   // 1st set is allowed
#endif
        if (!allowed) {
            allowed = check_hash_for_accseq(code_id, allowed_code_hashes, act.account, seq);
        }
        EOS_ASSERT(allowed, protected_contract_code, "can't change code of protected account");
    }

   auto ram_payer = context.get_storage_payer(act.account);
   chaindb.modify( account, ram_payer, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient to detect code version changes*/
      // TODO: update setcode message to include the hash, then validate it in validate
      a.last_code_update = context.control.pending_block_time();
      a.code_version = code_id;
      a.code.resize( code_size );
      if( code_size > 0 )
         memcpy( const_cast<char*>(a.code.data()), act.code.data(), code_size );

   });

   chaindb.modify( account_sequence, ram_payer, [&]( auto& aso ) {
      aso.code_sequence += 1;
   });

// TODO: Removed by CyberWay
//   if (new_size != old_size) {
//      context.add_ram_usage( act.account, new_size - old_size );
//   }
}

void apply_cyber_setabi(apply_context& context) {
   auto& chaindb  = context.chaindb;
   auto  act = context.act.data_as<setabi>();

   context.require_authorization(act.account);

   const auto& account = chaindb.get<account_object,by_name>(act.account);

   int64_t abi_size = act.abi.size();

   int64_t old_size = (int64_t)account.abi.size();
   int64_t new_size = abi_size;

    const auto& account_sequence = chaindb.get<account_sequence_object, by_name>(act.account);
    auto hash = bytes_hash(act.abi);
    if (is_protected_account(act.account)) {
        const auto seq = account_sequence.abi_sequence;
        bool allowed = false;
#ifdef ALLOW_INITIAL_CONTRACT_SET
        allowed = seq == 0;  // auto-enable initial set
#endif
        if (!allowed) {
            allowed = check_hash_for_accseq(hash, allowed_abi_hashes, act.account, seq);
        }
        EOS_ASSERT(allowed, protected_contract_code, "can't change abi of protected account");
    }

   auto ram_payer = context.get_storage_payer(act.account);

   chaindb.modify( account, ram_payer, [&]( auto& a ) {
      a.abi.resize( abi_size );
      if( abi_size > 0 )
         memcpy( const_cast<char*>(a.abi.data()), act.abi.data(), abi_size );
   });

   chaindb.modify( account_sequence, ram_payer, [&]( auto& aso ) {
      aso.abi_sequence += 1;
   });

   context.control.set_abi(act.account, account.get_abi());

// TODO: Removed by CyberWay
//   if (new_size != old_size) {
//      context.add_ram_usage( act.account, new_size - old_size );
//   }
}

void apply_cyber_updateauth(apply_context& context) {

   auto update = context.act.data_as<updateauth>();
   context.require_authorization(update.account); // only here to mark the single authority on this action as used

   auto& authorization = context.control.get_mutable_authorization_manager();
   auto& chaindb = context.chaindb;

   EOS_ASSERT(!update.permission.empty(), action_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT( update.permission.to_string().find(system_prefix()) != 0, action_validate_exception,
      "Permission names that start with '${prefix}' are reserved", ("prefix", system_prefix()));
   EOS_ASSERT(update.permission != update.parent, action_validate_exception, "Cannot set an authority as its own parent");
   chaindb.get<account_object, by_name>(update.account);
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

   auto ram_payer = context.get_storage_payer(update.account);

   if( permission ) {
      EOS_ASSERT(parent_id == permission->parent, action_validate_exception,
                 "Changing parent authority is not currently supported");


// TODO: Removed by CyberWay
//      int64_t old_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());

      authorization.modify_permission( *permission, ram_payer, update.auth );

// TODO: Removed by CyberWay
//      int64_t new_size = (int64_t)(config::billable_size_v<permission_object> + permission->auth.get_billable_size());
//      context.add_ram_usage( permission->owner, new_size - old_size );
   } else {
// TODO: Removed by CyberWay
//      const auto& p =

      authorization.create_permission( ram_payer, update.account, update.permission, parent_id, update.auth );

// TODO: Removed by CyberWay
//      int64_t new_size = (int64_t)(config::billable_size_v<permission_object> + p.auth.get_billable_size());
//      context.add_ram_usage( update.account, new_size );
   }
}

void apply_cyber_deleteauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto remove = context.act.data_as<deleteauth>();
   context.require_authorization(remove.account); // only here to mark the single authority on this action as used

   EOS_ASSERT(remove.permission != config::active_name, action_validate_exception, "Cannot delete active authority");
   EOS_ASSERT(remove.permission != config::owner_name, action_validate_exception, "Cannot delete owner authority");

   auto& authorization = context.control.get_mutable_authorization_manager();
   auto& chaindb = context.chaindb;



   { // Check for links to this permission
      auto link_table = chaindb.get_table<permission_link_object>();
      auto name_idx =  link_table.get_index<by_permission_name>();
      auto range = name_idx.equal_range(boost::make_tuple(remove.account, remove.permission));
      EOS_ASSERT(range.first == range.second, action_validate_exception,
                 "Cannot delete a linked authority. Unlink the authority first. This authority is linked to ${code}::${type}.", 
                 ("code", string(range.first->code))("type", string(range.first->message_type)));
   }

   const auto& permission = authorization.get_permission({remove.account, remove.permission});
// TODO: Removed by CyberWay
//   int64_t old_size = config::billable_size_v<permission_object> + permission.auth.get_billable_size();

   authorization.remove_permission( permission, context.get_storage_payer() );

// TODO: Removed by CyberWay
//   context.add_ram_usage( remove.account, -old_size );

}

void apply_cyber_linkauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto requirement = context.act.data_as<linkauth>();
   try {
      EOS_ASSERT(!requirement.requirement.empty(), action_validate_exception, "Required permission cannot be empty");

      context.require_authorization(requirement.account); // only here to mark the single authority on this action as used

      auto& chaindb = context.chaindb;
      const auto *account = chaindb.find<account_object, by_name>(requirement.account);
      EOS_ASSERT(account != nullptr, account_query_exception,
                 "Failed to retrieve account: ${account}", ("account", requirement.account)); // Redundant?
      const auto *code = chaindb.find<account_object, by_name>(requirement.code);
      EOS_ASSERT(code != nullptr, account_query_exception,
                 "Failed to retrieve code for account: ${account}", ("account", requirement.code));
      if( requirement.requirement != config::eosio_any_name ) {
         const auto *permission = chaindb.find<permission_object, by_name>(requirement.requirement);
         EOS_ASSERT(permission != nullptr, permission_query_exception,
                    "Failed to retrieve permission: ${permission}", ("permission", requirement.requirement));
      }

      auto link_key = boost::make_tuple(requirement.account, requirement.code, requirement.type);
      auto link = chaindb.find<permission_link_object, by_action_name>(link_key);
      auto ram_payer = context.get_storage_payer(requirement.account);

      if( link ) {
         EOS_ASSERT(link->required_permission != requirement.requirement, action_validate_exception,
                    "Attempting to update required authority, but new requirement is same as old");
         chaindb.modify(*link, ram_payer, [requirement = requirement.requirement](permission_link_object& link) {
             link.required_permission = requirement;
         });
      } else {
         chaindb.emplace<permission_link_object>(ram_payer, [&requirement](permission_link_object& link) {
            link.account = requirement.account;
            link.code = requirement.code;
            link.message_type = requirement.type;
            link.required_permission = requirement.requirement;
         });

// TODO: Removed by CyberWay
//         context.add_ram_usage(
//            requirement.account,
//            (int64_t)(config::billable_size_v<permission_link_object>)
//         );
      }

  } FC_CAPTURE_AND_RETHROW((requirement))
}

void apply_cyber_unlinkauth(apply_context& context) {
//   context.require_write_lock( config::eosio_auth_scope );

   auto& chaindb = context.chaindb;
   auto unlink = context.act.data_as<unlinkauth>();

   context.require_authorization(unlink.account); // only here to mark the single authority on this action as used

   auto link_key = boost::make_tuple(unlink.account, unlink.code, unlink.type);
   auto link = chaindb.find<permission_link_object, by_action_name>(link_key);
   EOS_ASSERT(link != nullptr, action_validate_exception, "Attempting to unlink authority, but no link found");
// TODO: Removed by CyberWay
//   context.add_ram_usage(
//      link->account,
//      -(int64_t)(config::billable_size_v<permission_link_object>)
//   );

   chaindb.erase(*link, context.get_storage_payer());
}

void apply_cyber_canceldelay(apply_context& context) {
   auto cancel = context.act.data_as<canceldelay>();
   context.require_authorization(cancel.canceling_auth.actor); // only here to mark the single authority on this action as used

   const auto& trx_id = cancel.trx_id;

   context.cancel_deferred_transaction(transaction_id_to_sender_id(trx_id), account_name());
}

} } // namespace eosio::chain
