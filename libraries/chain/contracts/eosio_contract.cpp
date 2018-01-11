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
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/staked_balance_objects.hpp>
#include <eosio/chain/contracts/producer_objects.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/producer_object.hpp>

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <eosio/chain/rate_limiting.hpp>

namespace eosio { namespace chain { namespace contracts {

void intialize_eosio_tokens(chainbase::database& db, const account_name& system_account, share_type initial_tokens) {
   const auto& t_id = db.create<contracts::table_id_object>([&](contracts::table_id_object &t_id){
      t_id.scope = system_account;
      t_id.code = config::system_account_name;
      t_id.table = N(currency);
   });

   db.create<key_value_object>([&](key_value_object &o){
      o.t_id = t_id.id;
      o.primary_key = N(account);
      o.value.insert(0, reinterpret_cast<const char *>(&initial_tokens), sizeof(share_type));
   });
}

static void modify_eosio_balance( apply_context& context, const account_name& account, share_type amt) {
   const auto& t_id = context.find_or_create_table(account, config::system_account_name, N(currency));
   uint64_t key = N(account);
   share_type balance = 0;
   context.front_record<key_value_index, by_scope_primary>(t_id, &key, (char *)&balance, sizeof(balance));

   balance += amt;

   context.store_record<key_value_object>(t_id, &key, (const char *)&balance, sizeof(balance));
}

share_type get_eosio_balance( const chainbase::database& db, const account_name &account ) {
   const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(account, config::system_account_name, N(currency)));
   if (!t_id) {
      return share_type(0);
   }

   const auto& idx = db.get_index<key_value_index, by_scope_primary>();
   auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
   if ( itr == idx.end() || itr->t_id != t_id->id ) {
      return share_type(0);
   }

   FC_ASSERT(itr->value.size() == sizeof(share_type), "Invalid data in EOSIO balance table");
   return *reinterpret_cast<const share_type *>(itr->value.data());
}

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
   try {
   context.require_authorization(create.creator);
   context.require_write_lock( config::eosio_auth_scope );

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

   const auto& new_account = db.create<account_object>([&create, &context](account_object& a) {
      a.name = create.name;
      a.creation_date = context.controller.head_block_time();
   });

   auto create_permission = [owner=create.name, &db, &context](const permission_name& name, permission_object::id_type parent, authority &&auth) {
      return db.create<permission_object>([&](permission_object& p) {
         p.name = name;
         p.parent = parent;
         p.owner = owner;
         p.auth = std::move(auth);
      });
   };

   const auto& owner_permission = create_permission("owner", 0, std::move(create.owner));
   create_permission("active", owner_permission.id, std::move(create.active));

   share_type creator_balance = get_eosio_balance(context.db, create.creator);

   EOS_ASSERT(creator_balance >= create.deposit.amount, action_validate_exception,
              "Creator '${c}' has insufficient funds to make account creation deposit of ${a}",
              ("c", create.creator)("a", create.deposit));

   modify_eosio_balance(context, create.creator, -create.deposit.amount);

   const auto& sbo = context.mutable_db.create<staked_balance_object>([&]( staked_balance_object& sbo) {
      sbo.owner_name = create.name;
      sbo.staked_balance = 0;
   });
   sbo.stake_tokens( create.deposit.amount, context.mutable_db );

   db.create<bandwidth_usage_object>([&]( auto& bu ) { bu.owner = create.name; });

} FC_CAPTURE_AND_RETHROW( (create) ) }

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
      context.require_write_lock(transfer.to);
      context.require_write_lock(transfer.from);

      context.require_authorization(transfer.from);

      context.require_recipient(transfer.to);
      context.require_recipient(transfer.from);
   } FC_CAPTURE_AND_RETHROW((transfer))


   try {
      auto& db = context.mutable_db;
      share_type from_balance = get_eosio_balance(db, transfer.from);

      EOS_ASSERT(from_balance >= transfer.amount, action_validate_exception, "Insufficient Funds",
                 ("from_balance", from_balance)("transfer.amount",transfer.amount));

      modify_eosio_balance(context, transfer.from, - share_type(transfer.amount) );
      modify_eosio_balance(context, transfer.to,     share_type(transfer.amount) );

   } FC_CAPTURE_AND_RETHROW( (transfer) ) 
}
///@}

/**
 *  Deduct the balance from the from account.
 */
void apply_eosio_lock(apply_context& context) {
   auto lock = context.act.as<contracts::lock>();

   EOS_ASSERT(lock.amount > 0, action_validate_exception, "Locked amount must be positive");

   context.require_write_lock(lock.to);
   context.require_write_lock(lock.from);
   context.require_write_lock(config::system_account_name);

   context.require_authorization(lock.from);

   context.require_recipient(lock.to);
   context.require_recipient(lock.from);

   share_type locker_balance = get_eosio_balance(context.db, lock.from);

   EOS_ASSERT( locker_balance >= lock.amount, action_validate_exception,
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount)("available",locker_balance) );

   modify_eosio_balance(context, lock.from, -share_type(lock.amount));

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
   context.require_write_lock( config::eosio_auth_scope );

   FC_ASSERT( act.vmtype == 0 );
   FC_ASSERT( act.vmversion == 0 );

   auto code_id = fc::sha256::hash( act.code.data(), act.code.size() );

   const auto& account = db.get<account_object,by_name>(act.account);
//   wlog( "set code: ${size}", ("size",act.code.size()));
   db.modify( account, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient to detect code version changes*/
      #warning TODO: update setcode message to include the hash, then validate it in validate 
      a.code_version = code_id;
      // Added resize(0) here to avoid bug in boost vector container
      a.code.resize( 0 );
      a.code.resize( act.code.size() );
      memcpy( a.code.data(), act.code.data(), act.code.size() );

   });

   // make sure the code gets a chance to initialize itself
   context.require_recipient(act.account);
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

   modify_eosio_balance(context, claim.account, share_type(claim.amount));
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

   context.require_write_lock(config::system_account_name);
   context.require_write_lock(approve.voter);
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
   context.require_write_lock( config::eosio_auth_scope );

   auto update = context.act.as<updateauth>();
   EOS_ASSERT(!update.permission.empty(), action_validate_exception, "Cannot create authority with empty name");
   EOS_ASSERT(update.permission != update.parent, action_validate_exception,
              "Cannot set an authority as its own parent");
   EOS_ASSERT(validate(update.data), action_validate_exception,
              "Invalid authority: ${auth}", ("auth", update.data));
   if (update.permission == "active")
      EOS_ASSERT(update.parent == "owner", action_validate_exception, "Cannot change active authority's parent from owner", ("update.parent", update.parent) );
   if (update.permission == "owner")
      EOS_ASSERT(update.parent.empty(), action_validate_exception, "Cannot change owner authority's parent");

   auto& db = context.mutable_db;

   FC_ASSERT(context.act.authorization.size(), "updateauth can only have one action authorization");
   const auto& act_auth = context.act.authorization.front();
   // lazy evaluating loop
   auto permission_is_valid_for_update = [&](){
      if (act_auth.permission == config::owner_name || act_auth.permission == update.permission) {
         return true;
      }

      auto current = db.get<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));
      while(current.name != config::owner_name) {
         if (current.name == act_auth.permission) {
            return true;
         }

         current = db.get<permission_object>(current.parent);
      }

      return false;
   };

   FC_ASSERT(act_auth.actor == update.account && permission_is_valid_for_update(), "updateauth must carry a permission equal to or in the ancestery of permission it updates");

   db.get<account_object, by_name>(update.account);
   validate_authority_precondition(context, update.data);

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
      db.modify(*permission, [&update, &parent_id, &context](permission_object& po) {
         po.auth = update.data;
         po.parent = parent_id;
         po.last_updated = context.controller.head_block_time();
      });
   }  else {
      // TODO/QUESTION: If we are creating a new permission, should we check if the message declared
      // permission satisfies the parent permission?
      db.create<permission_object>([&update, &parent_id, &context](permission_object& po) {
         po.name = update.permission;
         po.owner = update.account;
         po.auth = update.data;
         po.parent = parent_id;
         po.last_updated = context.controller.head_block_time();
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

void apply_eosio_onerror(apply_context& context) {
   assert(context.trx_meta.sender);
   context.require_recipient(*context.trx_meta.sender);
}

static const abi_serializer& get_abi_serializer() {
   static optional<abi_serializer> _abi_serializer;
   if (!_abi_serializer) {
      _abi_serializer.emplace(chain_initializer::eos_contract_abi());
   }

   return *_abi_serializer;
}

static optional<variant> get_pending_recovery(apply_context& context, account_name account ) {
   const auto* t_id = context.find_table(account, config::system_account_name, N(recovery));
   if (t_id) {
      uint64_t key = account;
      int32_t record_size = context.front_record<key_value_index, by_scope_primary>(*t_id, &key, nullptr, 0);
      if (record_size > 0) {
         bytes value(record_size + sizeof(uint64_t));
         uint64_t* key_p = reinterpret_cast<uint64_t *>(value.data());
         *key_p = key;

         record_size = context.front_record<key_value_index, by_scope_primary>(*t_id, &key, value.data() + sizeof(uint64_t), value.size() - sizeof(uint64_t));
         assert(record_size == value.size() - sizeof(uint64_t));

         return get_abi_serializer().binary_to_variant("pending_recovery", value);
      }
   }

   return optional<variant_object>();
}

static uint32_t get_next_sender_id(apply_context& context) {
   context.require_write_lock( config::eosio_auth_scope );
   const auto& t_id = context.find_or_create_table(config::eosio_auth_scope, config::system_account_name, N(deferred.seq));
   uint64_t key = N(config::eosio_auth_scope);
   uint32_t next_serial = 0;
   context.front_record<key_value_index, by_scope_primary>(t_id, &key, (char *)&next_serial, sizeof(uint32_t));

   uint32_t result = next_serial++;
   context.store_record<key_value_object>(t_id, &key, (char *)&next_serial, sizeof(uint32_t));
   return result;
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

   auto recover_act = context.act.as<postrecovery>();
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

   uint32_t request_id = get_next_sender_id(context);

   auto record_data = mutable_variant_object()
      ("account", account)
      ("request_id", request_id)
      ("update", update)
      ("memo", recover_act.memo);

   deferred_transaction dtrx;
   dtrx.sender = config::system_account_name;
   dtrx.sender_id = request_id;
   dtrx.region = 0;
   dtrx.execute_after = context.controller.head_block_time() + delay_lock;
   dtrx.set_reference_block(context.controller.head_block_id());
   dtrx.expiration = dtrx.execute_after + fc::seconds(60);
   dtrx.actions.emplace_back(vector<permission_level>{{account,config::active_name}},
                             passrecovery { account });

   context.execute_deferred(std::move(dtrx));


   const auto& t_id = context.find_or_create_table(account, config::system_account_name, N(recovery));
   auto data = get_abi_serializer().variant_to_binary("pending_recovery", record_data);
   context.store_record<key_value_object>(t_id,&account.value, data.data() + sizeof(uint64_t), data.size() - sizeof(uint64_t));

   context.console_append_formatted("Recovery Started for account ${account} : ${memo}\n", mutable_variant_object()("account", account)("memo", recover_act.memo));
}

static void remove_pending_recovery(apply_context& context, const account_name& account) {
   const auto& t_id = context.find_or_create_table(account, config::system_account_name, N(recovery));
   context.remove_record<key_value_object>(t_id, &account.value);
}

void apply_eosio_passrecovery(apply_context& context) {
   auto pass_act = context.act.as<passrecovery>();
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
   context.console_append_formatted("Account ${account} successfully recoverd!\n", mutable_variant_object()("account", account));
}

void apply_eosio_vetorecovery(apply_context& context) {
   context.require_write_lock( config::eosio_auth_scope );
   auto pass_act = context.act.as<vetorecovery>();
   const auto& account = pass_act.account;
   context.require_authorization(account);

   auto maybe_recovery = get_pending_recovery(context, account);
   FC_ASSERT(maybe_recovery, "No pending recovery found for account ${account}", ("account", account));
   auto recovery = *maybe_recovery;

   context.cancel_deferred(recovery["request_id"].as<uint32_t>());

   remove_pending_recovery(context, account);
   context.console_append_formatted("Recovery for account ${account} vetoed!\n", mutable_variant_object()("account", account));
}


} } } // namespace eosio::chain::contracts
