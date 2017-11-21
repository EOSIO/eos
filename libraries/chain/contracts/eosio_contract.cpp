/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/eos_contract.hpp>

#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/contracts/balance_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/staked_balance_objects.hpp>
#include <eosio/chain/contracts/producer_objects.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/producer_object.hpp>

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

namespace eosio { namespace chain { namespace contracts {

void validate_authority_precondition( const apply_context& context, const authority& auth ) {
   for(const auto& a : auth.accounts) {
      context.db.get<account_object, by_name>(a.permission.actor);
      context.db.find<permission_object, by_owner>(boost::make_tuple(a.permission.actor, a.permission.permission));
   }
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds and proceeds to
 *  deduct the balance of the account creator by deposit, this deposit is supposed to be
 *  credited to the staked balance the new account in the @staked contract.
 */
void apply_eosio_newaccount(apply_context& context) {
   auto create = context.act.as<newaccount>();
   context.require_authorization(create.creator);
   context.require_write_scope( config::eosio_auth_scope );

   EOS_ASSERT( validate(create.owner), action_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), action_validate_exception, "Invalid active authority");
   EOS_ASSERT( validate(create.recovery), action_validate_exception, "Invalid recovery authority");

   auto& db = context.mutable_db;

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, action_validate_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   for( const auto& auth : { create.owner, create.active, create.recovery } ){
      validate_authority_precondition( context, auth );
   }

   const auto& new_account = db.create<account_object>([&create, &db](account_object& a) {
      a.name = create.name;
      a.creation_date = db.get(dynamic_global_property_object::id_type()).time;
   });

   const auto& owner_permission = db.create<permission_object>([&create, &new_account](permission_object& p) {
      p.name = "owner";
      p.parent = 0;
      p.owner = new_account.name;
      p.auth = std::move(create.owner);
   });
   db.create<permission_object>([&create, &owner_permission](permission_object& p) {
      p.name = "active";
      p.parent = owner_permission.id;
      p.owner = owner_permission.owner;
      p.auth = std::move(create.active);
   });

   const auto& creator_balance = context.mutable_db.get<balance_object, by_owner_name>(create.creator);

   EOS_ASSERT(creator_balance.balance >= create.deposit.amount, action_validate_exception,
              "Creator '${c}' has insufficient funds to make account creation deposit of ${a}",
              ("c", create.creator)("a", create.deposit));

   context.mutable_db.modify(creator_balance, [&create](balance_object& b) {
      b.balance -= create.deposit.amount;
   });

   context.mutable_db.create<balance_object>([&create](balance_object& b) {
      b.owner_name = create.name;
      b.balance = 0; //create.deposit.amount; TODO: make sure we credit this in @staked
   });

   context.mutable_db.create<staked_balance_object>([&create](staked_balance_object& sbo) {
      sbo.owner_name = create.name;
      sbo.staked_balance = create.deposit.amount;
   });
}

/**
 *
 * @ingroup native_eos
 * @defgroup eos_eosio_transfer eosio::eos_transfer
 */
///@{


void apply_eosio_transfer(apply_context& context) {
   auto transfer = context.act.as<contracts::transfer>();

   try {
      EOS_ASSERT(transfer.amount > 0, action_validate_exception, "Must transfer a positive amount");
      context.require_write_scope(transfer.to);
      context.require_write_scope(transfer.from);

      context.require_authorization(transfer.from);

      context.require_recipient(transfer.to);
      context.require_recipient(transfer.from);
   } FC_CAPTURE_AND_RETHROW((transfer))


   try {
      auto& db = context.mutable_db;
      const auto& from = db.get<balance_object, by_owner_name>(transfer.from);

      EOS_ASSERT(from.balance >= transfer.amount, action_validate_exception, "Insufficient Funds",
                 ("from.balance",from.balance)("transfer.amount",transfer.amount));

      const auto& to = db.get<balance_object, by_owner_name>(transfer.to);
      db.modify(from, [&](balance_object& a) {
         a.balance -= share_type(transfer.amount);
      });
      db.modify(to, [&](balance_object& a) {
         a.balance += share_type(transfer.amount);
      });
   } FC_CAPTURE_AND_RETHROW( (transfer) ) 
}
///@}

/**
 *  Deduct the balance from the from account.
 */
void apply_eosio_lock(apply_context& context) {
   auto lock = context.act.as<contracts::lock>();

   EOS_ASSERT(lock.amount > 0, action_validate_exception, "Locked amount must be positive");

   context.require_write_scope(lock.to);
   context.require_write_scope(lock.from);
   context.require_write_scope(config::system_account_name);

   context.require_authorization(lock.from);

   context.require_recipient(lock.to);
   context.require_recipient(lock.from);

   const auto& locker = context.db.get<balance_object, by_owner_name>(lock.from);

   EOS_ASSERT( locker.balance >= lock.amount, action_validate_exception, 
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount)("available",locker.balance) );

   context.mutable_db.modify(locker, [&lock](balance_object& a) {
      a.balance -= lock.amount;
   });

   const auto& balance = context.db.get<staked_balance_object, by_owner_name>(lock.to);
   balance.stake_tokens(lock.amount, context.mutable_db);
}

void apply_eosio_unlock(apply_context& context) {
   auto unlock = context.act.as<contracts::unlock>();

   context.require_authorization(unlock.account);

   EOS_ASSERT(unlock.amount >= 0, action_validate_exception, "Unlock amount cannot be negative");

   const auto& balance = context.db.get<staked_balance_object, by_owner_name>(unlock.account);

   EOS_ASSERT(balance.staked_balance  >= unlock.amount, action_validate_exception,
              "Insufficient locked funds to unlock ${a}", ("a", unlock.amount));

   balance.begin_unstaking_tokens(unlock.amount, context.mutable_db);
}


void apply_eosio_setcode(apply_context& context) {
   auto& db = context.mutable_db;
   auto  act = context.act.as<setcode>();

   context.require_authorization(act.account);
   context.require_write_scope( config::eosio_auth_scope );

   FC_ASSERT( act.vmtype == 0 );
   FC_ASSERT( act.vmversion == 0 );

   auto code_id = fc::sha256::hash( act.code.data(), act.code.size() );

   const auto& account = db.get<account_object,by_name>(act.account);
//   wlog( "set code: ${size}", ("size",act.code.size()));
   db.modify( account, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient to detect code version changes*/
      #warning TODO: update setcode message to include the hash, then validate it in validate 
      a.code_version = code_id;
      a.code.resize( act.code.size() );
      memcpy( a.code.data(), act.code.data(), act.code.size() );

   });

   // create an apply context for initialization
   apply_context init_context( context.mutable_controller, context.mutable_db, context.trx, context.act, act.account );

   // get code from cache
   auto code = context.mutable_controller.get_wasm_cache().checkout_scoped(code_id, act.code.data(), act.code.size());
   wasm_interface::get().init( code, init_context );
}

void apply_eosio_setabi(apply_context& context) {
   auto& db = context.mutable_db;
   auto  act = context.act.as<setabi>();

   context.require_authorization(act.account);

   /// if an ABI is specified make sure it is well formed and doesn't
   /// reference any undefined types
   abi_serializer(act.abi).validate();
   // todo: figure out abi serilization location

   const auto& account = db.get<account_object,by_name>(act.account);
   db.modify( account, [&]( auto& a ) {
      a.set_abi( act.abi );
   });
}

void apply_eosio_claim(apply_context& context) {
   auto claim = context.act.as<contracts::claim>();

   EOS_ASSERT(claim.amount > 0, action_validate_exception, "Claim amount must be positive");

   context.require_authorization(claim.account);

   auto balance = context.db.find<staked_balance_object, by_owner_name>(claim.account);

   EOS_ASSERT(balance != nullptr, action_validate_exception,
              "Could not find staked balance for ${name}", ("name", claim.account));

   auto balance_release_time = balance->last_unstaking_time + fc::seconds(config::staked_balance_cooldown_sec);
   auto now = context.controller.head_block_time();

   EOS_ASSERT(now >= balance_release_time, action_validate_exception,
              "Cannot claim balance until ${releaseDate}", ("releaseDate", balance_release_time));
   EOS_ASSERT(balance->unstaking_balance >= claim.amount, action_validate_exception,
              "Cannot claim ${claimAmount} as only ${available} is available for claim",
              ("claimAmount", claim.amount)("available", balance->unstaking_balance));

   const auto& staked_balance = context.db.get<staked_balance_object, by_owner_name>(claim.account);
   staked_balance.finish_unstaking_tokens(claim.amount, context.mutable_db);

   const auto& liquid_balance = context.db.get<balance_object, by_owner_name>(claim.account);
   context.mutable_db.modify(liquid_balance, [&claim](balance_object& a) {
      a.balance += claim.amount;
   });
}


void apply_eosio_setproducer(apply_context& context) {
   auto update = context.act.as<setproducer>();
   context.require_authorization(update.name);
   EOS_ASSERT(update.name.good(), action_validate_exception, "Producer owner name cannot be empty");

   auto& db = context.mutable_db;
   auto producer = db.find<producer_object, by_owner>(update.name);

   if (producer) {
      EOS_ASSERT(producer->signing_key != update.key || producer->configuration != update.configuration,
                 action_validate_exception, "Producer's new settings may not be identical to old settings");

      db.modify(*producer, [&](producer_object& p) {
         p.signing_key = update.key;
         p.configuration = update.configuration;
      });
   } else {
      db.create<producer_object>([&](producer_object& p) {
         p.owner = update.name;
         p.signing_key = update.key;
         p.configuration = update.configuration;
      });
      db.create<producer_votes_object>([&](producer_votes_object& pvo) {
         pvo.owner_name = update.name;
      });
   }
}


void apply_eosio_okproducer(apply_context& context) {
   auto approve = context.act.as<okproducer>();
   EOS_ASSERT(approve.approve == 0 || approve.approve == 1, action_validate_exception,
              "Unknown approval value: ${val}; must be either 0 or 1", ("val", approve.approve));
   context.require_recipient(approve.voter);
   context.require_recipient(approve.producer);

   context.require_write_scope(config::system_account_name);
   context.require_write_scope(approve.voter);
   context.require_authorization(approve.voter);


   auto& db = context.mutable_db;
   const auto& producer = db.get<producer_votes_object, by_owner_name>(approve.producer);
   const auto& voter = db.get<staked_balance_object, by_owner_name>(approve.voter);


   EOS_ASSERT(voter.producer_votes.contains<producer_slate>(), action_validate_exception,
              "Cannot approve producer; approving account '${name}' proxies its votes to '${proxy}'",
              ("name", voter.owner_name)("proxy", voter.producer_votes.get<account_name>()));


   const auto& slate = voter.producer_votes.get<producer_slate>();

   EOS_ASSERT(slate.size < config::max_producer_votes, action_validate_exception,
              "Cannot approve producer; approved producer count is already at maximum");
   if (approve.approve)
      EOS_ASSERT(!slate.contains(producer.owner_name), action_validate_exception,
                 "Cannot add approval to producer '${name}'; producer is already approved",
                 ("name", producer.owner_name));
   else
      EOS_ASSERT(slate.contains(producer.owner_name), action_validate_exception,
                 "Cannot remove approval from producer '${name}'; producer is not approved",
                 ("name", producer.owner_name));


   auto total_voting_stake = voter.staked_balance;

   // Check if voter is proxied to; if so, we need to add in the proxied stake
   if (auto proxy = db.find<proxy_vote_object, by_target_name>(voter.owner_name))
      total_voting_stake += proxy->proxied_stake;

   // Add/remove votes from producer
   db.modify(producer, [approve = approve.approve, total_voting_stake](producer_votes_object& pvo) {
      if (approve)
         pvo.update_votes(total_voting_stake);
      else
         pvo.update_votes(-total_voting_stake);
   });
   // Add/remove producer from voter's approved producer list
   db.modify(voter, [&approve, producer = producer.owner_name](staked_balance_object& sbo) {
      auto& slate = sbo.producer_votes.get<producer_slate>();
      if (approve.approve)
         slate.add(producer);
      else
         slate.remove(producer);
   });
}

void apply_eosio_setproxy(apply_context& context) {
   auto svp = context.act.as<setproxy>();
   FC_ASSERT( !"Not Implemented Yet" );

   /*
   context.require_authorization( spv.stakeholder );

   context.require_recipient(svp.stakeholder);
   context.require_recipient(svp.proxy);

   auto& db = context.mutable_db;
   const auto& proxy = db.get<proxy_vote_object, by_target_name>(context.act.recipient(svp.proxy));
   const auto& balance = db.get<staked_balance_object, by_owner_name>(context.act.recipient(svp.stakeholder));


   auto proxy = db.find<proxy_vote_object, by_target_name>(context.act.recipient(svp.proxy));


   if (svp.proxy != svp.stakeholder) {
      // We are enabling proxying to svp.proxy
      proxy.add_proxy_source(context.act.recipient(svp.stakeholder), balance.staked_balance, db);
      db.modify(balance, [target = proxy.proxy_target](staked_balance_object& sbo) { sbo.producer_votes = target; });
   } else {
      // We are disabling proxying to balance.producer_votes.get<account_name>()
      proxy.remove_proxy_source(context.act.recipient(svp.stakeholder), balance.staked_balance, db);
      db.modify(balance, [](staked_balance_object& sbo) { sbo.producer_votes = producer_slate{}; });
   }
   */
}

void apply_eosio_updateauth(apply_context& context) {
   context.require_write_scope( config::eosio_auth_scope );

   auto update = context.act.as<updateauth>();
   EOS_ASSERT(!update.permission.empty(), action_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT(update.permission != update.parent, action_validate_exception,
              "Cannot set an authority as its own parent");
   EOS_ASSERT(validate(update.authority), action_validate_exception,
              "Invalid authority: ${auth}", ("auth", update.authority));
   if (update.permission == "active")
      EOS_ASSERT(update.parent == "owner", action_validate_exception, "Cannot change active authority's parent from owner", ("update.parent", update.parent) );
   if (update.permission == "owner")
      EOS_ASSERT(update.parent.empty(), action_validate_exception, "Cannot change owner authority's parent");

   auto& db = context.mutable_db;
   context.require_authorization(update.account);

   db.get<account_object, by_name>(update.account);
   validate_authority_precondition(context, update.authority);

   auto permission = db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));
   
   permission_object::id_type parent_id = 0;
   if(update.permission != "owner") {
      auto& parent = db.get<permission_object, by_owner>(boost::make_tuple(update.account, update.parent));
      parent_id = parent.id;
   }

   if (permission) {
      EOS_ASSERT(parent_id == permission->parent, action_validate_exception,
                 "Changing parent authority is not currently supported");
   
      // TODO/QUESTION: If we are updating an existing permission, should we check if the message declared
      // permission satisfies the permission we want to modify?
      db.modify(*permission, [&update, &parent_id](permission_object& po) {
         po.auth = update.authority;
         po.parent = parent_id;
      });
   }  else {
      // TODO/QUESTION: If we are creating a new permission, should we check if the message declared
      // permission satisfies the parent permission?
      db.create<permission_object>([&update, &parent_id](permission_object& po) {
         po.name = update.permission;
         po.owner = update.account;
         po.auth = update.authority;
         po.parent = parent_id;
      });
   }
}

void apply_eosio_deleteauth(apply_context& context) {
   auto remove = context.act.as<deleteauth>();
   EOS_ASSERT(remove.permission != "active", action_validate_exception, "Cannot delete active authority");
   EOS_ASSERT(remove.permission != "owner", action_validate_exception, "Cannot delete owner authority");

   auto& db = context.mutable_db;
   context.require_authorization(remove.account);
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

   if (context.controller.is_applying_block())
      db.remove(permission);
}

void apply_eosio_linkauth(apply_context& context) {
   auto requirement = context.act.as<linkauth>();
   EOS_ASSERT(!requirement.requirement.empty(), action_validate_exception, "Required permission cannot be empty");

   context.require_authorization(requirement.account);
   
   auto& db = context.mutable_db;
   db.get<account_object, by_name>(requirement.account);
   db.get<account_object, by_name>(requirement.code);
   db.get<permission_object, by_name>(requirement.requirement);
   
   auto link_key = boost::make_tuple(requirement.account, requirement.code, requirement.type);
   auto link = db.find<permission_link_object, by_action_name>(link_key);
   
   if (link) {
      EOS_ASSERT(link->required_permission != requirement.requirement, action_validate_exception,
                 "Attempting to update required authority, but new requirement is same as old");
      if (context.controller.is_applying_block())
         db.modify(*link, [requirement = requirement.requirement](permission_link_object& link) {
            link.required_permission = requirement;
         });
   } else if (context.controller.is_applying_block()) {
      db.create<permission_link_object>([&requirement](permission_link_object& link) {
         link.account = requirement.account;
         link.code = requirement.code;
         link.message_type = requirement.type;
         link.required_permission = requirement.requirement;
      });
   }
}

void apply_eosio_unlinkauth(apply_context& context) {
   auto& db = context.mutable_db;
   auto unlink = context.act.as<unlinkauth>();

   context.require_authorization(unlink.account);

   auto link_key = boost::make_tuple(unlink.account, unlink.code, unlink.type);
   auto link = db.find<permission_link_object, by_action_name>(link_key);
   EOS_ASSERT(link != nullptr, action_validate_exception, "Attempting to unlink authority, but no link found");
   if (context.controller.is_applying_block())
      db.remove(*link);
}

void apply_eosio_nonce(apply_context&) {
   /// do nothing
}

} } } // namespace eosio::chain::contracts
