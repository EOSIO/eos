#include <eos/native_contract/eos_contract.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/message.hpp>
#include <eos/chain/exceptions.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/native_contract/balance_object.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/permission_link_object.hpp>
#include <eos/chain/global_property_object.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/producer_objects.hpp>
#include <eos/chain/producer_object.hpp>

#include <eos/chain/wasm_interface.hpp>
#include <eos/types/AbiSerializer.hpp>

namespace native {
namespace eos {
using namespace ::eos::chain;
namespace config = ::eos::config;
namespace chain = ::eos::chain;
using namespace ::eos::types;

void validate_authority_precondition( const apply_context& context, const Authority& auth ) {
   for(const auto& a : auth.accounts)
      context.db.get<account_object,by_name>(a.permission.account);
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds and proceeds to
 *  deduct the balance of the account creator by deposit, this deposit is supposed to be
 *  credited to the staked balance the new account in the @staked contract.
 */
void apply_eos_newaccount(apply_context& context) {
   auto create = context.msg.as<types::newaccount>();
   context.require_authorization(create.creator);

   EOS_ASSERT( validate(create.owner), message_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), message_validate_exception, "Invalid active authority");
   EOS_ASSERT( validate(create.recovery), message_validate_exception, "Invalid recovery authority");

   auto& db = context.mutable_db;

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, message_precondition_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   for( const auto& auth : { create.owner, create.active, create.recovery } ){
      validate_authority_precondition( context, auth );
   }

   const auto& new_account = db.create<account_object>([&create, &db](account_object& a) {
      a.name = create.name;
      a.creation_date = db.get(dynamic_global_property_object::id_type()).time;
   });
   if (context.controller.is_applying_block()) {
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
   }

   const auto& creatorBalance = context.mutable_db.get<BalanceObject, byOwnerName>(create.creator);

   EOS_ASSERT(creatorBalance.balance >= create.deposit.amount, message_validate_exception,
              "Creator '${c}' has insufficient funds to make account creation deposit of ${a}",
              ("c", create.creator)("a", create.deposit));

   context.mutable_db.modify(creatorBalance, [&create](BalanceObject& b) {
      b.balance -= create.deposit.amount;
   });

   context.mutable_db.create<BalanceObject>([&create](BalanceObject& b) {
      b.ownerName = create.name;
      b.balance = 0; //create.deposit.amount; TODO: make sure we credit this in @staked
   });

   context.mutable_db.create<StakedBalanceObject>([&create](StakedBalanceObject& sbo) {
      sbo.ownerName = create.name;
      sbo.stakedBalance = create.deposit.amount;
   });
}

/**
 *
 * @ingroup native_eos
 * @defgroup eos_eos_transfer eos::eos_transfer
 */
///@{


void apply_eos_transfer(apply_context& context) {
   auto transfer = context.msg.as<types::transfer>();

   try {
      EOS_ASSERT(transfer.amount > 0, message_validate_exception, "Must transfer a positive amount");
      context.require_scope(transfer.to);
      context.require_scope(transfer.from);

      context.require_authorization(transfer.from);

      context.require_recipient(transfer.to);
      context.require_recipient(transfer.from);
   } FC_CAPTURE_AND_RETHROW((transfer))


   try {
      auto& db = context.mutable_db;
      const auto& from = db.get<BalanceObject, byOwnerName>(transfer.from);

      EOS_ASSERT(from.balance >= transfer.amount, message_precondition_exception, "Insufficient Funds",
                 ("from.balance",from.balance)("transfer.amount",transfer.amount));

      const auto& to = db.get<BalanceObject, byOwnerName>(transfer.to);
      db.modify(from, [&](BalanceObject& a) {
         a.balance -= ShareType(transfer.amount);
      });
      db.modify(to, [&](BalanceObject& a) {
         a.balance += ShareType(transfer.amount);
      });
   } FC_CAPTURE_AND_RETHROW( (transfer) ) 
}
///@}

/**
 *  Deduct the balance from the from account.
 */
void apply_eos_lock(apply_context& context) {
   auto lock = context.msg.as<types::lock>();

   EOS_ASSERT(lock.amount > 0, message_validate_exception, "Locked amount must be positive");

   context.require_scope(lock.to);
   context.require_scope(lock.from);
   context.require_scope(config::EosContractName);

   context.require_authorization(lock.from);

   context.require_recipient(lock.to);
   context.require_recipient(lock.from);

   context.require_authorization(lock.from);

   const auto& locker = context.db.get<BalanceObject, byOwnerName>(lock.from);

   EOS_ASSERT( locker.balance >= lock.amount, message_precondition_exception, 
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount)("available",locker.balance) );

   context.mutable_db.modify(locker, [&lock](BalanceObject& a) {
      a.balance -= lock.amount;
   });

   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(lock.to);
   balance.stakeTokens(lock.amount, context.mutable_db);
}

void apply_eos_unlock(apply_context& context) {
   auto unlock = context.msg.as<types::unlock>();

   context.require_authorization(unlock.account);

   EOS_ASSERT(unlock.amount >= 0, message_validate_exception, "Unlock amount cannot be negative");

   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(unlock.account);

   EOS_ASSERT(balance.stakedBalance  >= unlock.amount, message_precondition_exception,
              "Insufficient locked funds to unlock ${a}", ("a", unlock.amount));

   balance.beginUnstakingTokens(unlock.amount, context.mutable_db);
}


void apply_eos_setcode(apply_context& context) {
   auto& db = context.mutable_db;
   auto  msg = context.msg.as<types::setcode>();

   context.require_authorization(msg.account);

   FC_ASSERT( msg.vmtype == 0 );
   FC_ASSERT( msg.vmversion == 0 );

   /// if an ABI is specified make sure it is well formed and doesn't
   /// reference any undefined types
   AbiSerializer( msg.abi ).validate();


   const auto& account = db.get<account_object,by_name>(msg.account);
//   wlog( "set code: ${size}", ("size",msg.code.size()));
   db.modify( account, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient to detect code version changes*/
      #warning TODO: update setcode message to include the hash, then validate it in validate 
      a.code_version = fc::sha256::hash( msg.code.data(), msg.code.size() );
      a.code.resize( msg.code.size() );
      memcpy( a.code.data(), msg.code.data(), msg.code.size() );

      a.set_abi( msg.abi );
   });

   apply_context init_context( context.mutable_controller, context.mutable_db, context.trx, context.msg, msg.account );
   wasm_interface::get().init( init_context );
}

void apply_eos_claim(apply_context& context) {
   auto claim = context.msg.as<types::claim>();

   EOS_ASSERT(claim.amount > 0, message_validate_exception, "Claim amount must be positive");

   context.require_authorization(claim.account);

   auto balance = context.db.find<StakedBalanceObject, byOwnerName>(claim.account);
   EOS_ASSERT(balance != nullptr, message_precondition_exception,
              "Could not find staked balance for ${name}", ("name", claim.account));
   auto balanceReleaseTime = balance->lastUnstakingTime + config::StakedBalanceCooldownSeconds;
   auto now = context.controller.head_block_time();
   EOS_ASSERT(now >= balanceReleaseTime, message_precondition_exception,
              "Cannot claim balance until ${releaseDate}", ("releaseDate", balanceReleaseTime));
   EOS_ASSERT(balance->unstakingBalance >= claim.amount, message_precondition_exception,
              "Cannot claim ${claimAmount} as only ${available} is available for claim",
              ("claimAmount", claim.amount)("available", balance->unstakingBalance));

   const auto& stakedBalance = context.db.get<StakedBalanceObject, byOwnerName>(claim.account);
   stakedBalance.finishUnstakingTokens(claim.amount, context.mutable_db);

   const auto& liquidBalance = context.db.get<BalanceObject, byOwnerName>(claim.account);
   context.mutable_db.modify(liquidBalance, [&claim](BalanceObject& a) {
      a.balance += claim.amount;
   });
}


void apply_eos_setproducer(apply_context& context) {
   auto update = context.msg.as<types::setproducer>();
   context.require_authorization(update.name);
   EOS_ASSERT(update.name.good(), message_validate_exception, "Producer owner name cannot be empty");

   auto& db = context.mutable_db;
   auto producer = db.find<producer_object, by_owner>(update.name);

   if (producer) {
      EOS_ASSERT(producer->signing_key != update.key || producer->configuration != update.configuration,
                 message_validate_exception, "Producer's new settings may not be identical to old settings");

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
      auto raceTime = ProducerScheduleObject::get(db).currentRaceTime;
      db.create<ProducerVotesObject>([&](ProducerVotesObject& pvo) {
         pvo.ownerName = update.name;
         pvo.startNewRaceLap(raceTime);
      });
   }
}


void apply_eos_okproducer(apply_context& context) {
   auto approve = context.msg.as<types::okproducer>();
   EOS_ASSERT(approve.approve == 0 || approve.approve == 1, message_validate_exception,
              "Unknown approval value: ${val}; must be either 0 or 1", ("val", approve.approve));
   context.require_recipient(approve.voter);
   context.require_recipient(approve.producer);

   context.require_scope(config::EosContractName);
   context.require_scope(approve.voter);
   context.require_authorization(approve.voter);


   auto& db = context.mutable_db;
   const auto& producer = db.get<ProducerVotesObject, byOwnerName>(approve.producer);
   const auto& voter = db.get<StakedBalanceObject, byOwnerName>(approve.voter);


   EOS_ASSERT(voter.producerVotes.contains<ProducerSlate>(), message_precondition_exception,
              "Cannot approve producer; approving account '${name}' proxies its votes to '${proxy}'",
              ("name", voter.ownerName)("proxy", voter.producerVotes.get<AccountName>()));


   const auto& slate = voter.producerVotes.get<ProducerSlate>();

   EOS_ASSERT(slate.size < config::MaxProducerVotes, message_precondition_exception,
              "Cannot approve producer; approved producer count is already at maximum");
   if (approve.approve)
      EOS_ASSERT(!slate.contains(producer.ownerName), message_precondition_exception,
                 "Cannot add approval to producer '${name}'; producer is already approved",
                 ("name", producer.ownerName));
   else
      EOS_ASSERT(slate.contains(producer.ownerName), message_precondition_exception,
                 "Cannot remove approval from producer '${name}'; producer is not approved",
                 ("name", producer.ownerName));


   auto raceTime = ProducerScheduleObject::get(db).currentRaceTime;
   auto totalVotingStake = voter.stakedBalance;

   // Check if voter is proxied to; if so, we need to add in the proxied stake
   if (auto proxy = db.find<ProxyVoteObject, byTargetName>(voter.ownerName))
      totalVotingStake += proxy->proxiedStake;

   // Add/remove votes from producer
   db.modify(producer, [approve = approve.approve, totalVotingStake, &raceTime](ProducerVotesObject& pvo) {
      if (approve)
         pvo.updateVotes(totalVotingStake, raceTime);
      else
         pvo.updateVotes(-totalVotingStake, raceTime);
   });
   // Add/remove producer from voter's approved producer list
   db.modify(voter, [&approve, producer = producer.ownerName](StakedBalanceObject& sbo) {
      auto& slate = sbo.producerVotes.get<ProducerSlate>();
      if (approve.approve)
         slate.add(producer);
      else
         slate.remove(producer);
   });
}

void apply_eos_setproxy(apply_context& context) {
   auto svp = context.msg.as<types::setproxy>();
   FC_ASSERT( !"Not Implemented Yet" );

   /*
   context.require_authorization( spv.stakeholder );

   context.require_recipient(svp.stakeholder);
   context.require_recipient(svp.proxy);

   auto& db = context.mutable_db;
   const auto& proxy = db.get<ProxyVoteObject, byTargetName>(context.msg.recipient(svp.proxy));
   const auto& balance = db.get<StakedBalanceObject, byOwnerName>(context.msg.recipient(svp.stakeholder));


   auto proxy = db.find<ProxyVoteObject, byTargetName>(context.msg.recipient(svp.proxy));


   if (svp.proxy != svp.stakeholder) {
      // We are enabling proxying to svp.proxy
      proxy.addProxySource(context.msg.recipient(svp.stakeholder), balance.stakedBalance, db);
      db.modify(balance, [target = proxy.proxyTarget](StakedBalanceObject& sbo) { sbo.producerVotes = target; });
   } else {
      // We are disabling proxying to balance.producerVotes.get<AccountName>()
      proxy.removeProxySource(context.msg.recipient(svp.stakeholder), balance.stakedBalance, db);
      db.modify(balance, [](StakedBalanceObject& sbo) { sbo.producerVotes = ProducerSlate{}; });
   }
   */
}

void apply_eos_updateauth(apply_context& context) {
   auto update = context.msg.as<types::updateauth>();
   EOS_ASSERT(!update.permission.empty(), message_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT(update.permission != update.parent, message_validate_exception,
              "Cannot set an authority as its own parent");
   EOS_ASSERT(validate(update.authority), message_validate_exception,
              "Invalid authority: ${auth}", ("auth", update.authority));
   if (update.permission == "active")
      EOS_ASSERT(update.parent == "owner", message_validate_exception, "Cannot change active authority's parent");
   if (update.permission == "owner")
      EOS_ASSERT(update.parent.empty(), message_validate_exception, "Cannot change owner authority's parent");

   auto& db = context.mutable_db;
   context.require_authorization(update.account);

   db.get<account_object, by_name>(update.account);
   for (auto accountPermission : update.authority.accounts) {
      db.get<account_object, by_name>(accountPermission.permission.account);
      db.get<permission_object, by_owner>(boost::make_tuple(accountPermission.permission.account,
                                                            accountPermission.permission.permission));
   }

   auto permission = db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));
   auto& parent = db.get<permission_object, by_owner>(boost::make_tuple(update.account, update.parent));
   if (permission) {
      EOS_ASSERT(parent.id == permission->parent, message_precondition_exception,
                 "Changing parent authority is not currently supported");
      if (context.controller.is_applying_block())
         db.modify(*permission, [&update, parent = parent.id](permission_object& po) {
            po.auth = update.authority;
            po.parent = parent;
         });
   } else if (context.controller.is_applying_block()) {
      db.create<permission_object>([&update, parent = parent.id](permission_object& po) {
         po.name = update.permission;
         po.owner = update.account;
         po.auth = update.authority;
         po.parent = parent;
      });
   }
}

void apply_eos_deleteauth(apply_context& context) {
   auto remove = context.msg.as<types::deleteauth>();
   EOS_ASSERT(remove.permission != "active", message_validate_exception, "Cannot delete active authority");
   EOS_ASSERT(remove.permission != "owner", message_validate_exception, "Cannot delete owner authority");

   auto& db = context.mutable_db;
   context.require_authorization(remove.account);
   const auto& permission = db.get<permission_object, by_owner>(boost::make_tuple(remove.account, remove.permission));

   { // Check for children
      const auto& index = db.get_index<permission_index, by_parent>();
      auto range = index.equal_range(permission.id);
      EOS_ASSERT(range.first == range.second, message_precondition_exception,
                 "Cannot delete an authority which has children. Delete the children first");
   }

   { // Check for links to this permission
      const auto& index = db.get_index<permission_link_index, by_permission_name>();
      auto range = index.equal_range(boost::make_tuple(remove.account, remove.permission));
      EOS_ASSERT(range.first == range.second, message_precondition_exception,
                 "Cannot delete a linked authority. Unlink the authority first");
   }

   if (context.controller.is_applying_block())
      db.remove(permission);
}

void apply_eos_linkauth(apply_context& context) {
   auto requirement = context.msg.as<types::linkauth>();
   EOS_ASSERT(!requirement.requirement.empty(), message_validate_exception, "Required permission cannot be empty");

   context.require_authorization(requirement.account);
   
   auto& db = context.mutable_db;
   db.get<account_object, by_name>(requirement.account);
   db.get<account_object, by_name>(requirement.code);
   db.get<permission_object, by_name>(requirement.requirement);
   
   auto linkKey = boost::make_tuple(requirement.account, requirement.code, requirement.type);
   auto link = db.find<permission_link_object, by_message_type>(linkKey);
   
   if (link) {
      EOS_ASSERT(link->required_permission != requirement.requirement, message_precondition_exception,
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

void apply_eos_unlinkauth(apply_context& context) {
   auto& db = context.mutable_db;
   auto unlink = context.msg.as<types::unlinkauth>();

   context.require_authorization(unlink.account);

   auto linkKey = boost::make_tuple(unlink.account, unlink.code, unlink.type);
   auto link = db.find<permission_link_object, by_message_type>(linkKey);
   EOS_ASSERT(link != nullptr, message_precondition_exception, "Attempting to unlink authority, but no link found");
   if (context.controller.is_applying_block())
      db.remove(*link);
}

} // namespace eos
} // namespace native
