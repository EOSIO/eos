/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/objects.hpp>
#include <eos/native_contract/eos_contract.hpp>

#include <eos/chain/producer_object.hpp>
#include <eos/chain/permission_object.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace eosio { namespace native_contract {
using namespace eosio::chain;

types::time native_contract_chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

blockchain_configuration native_contract_chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

std::array<types::account_name, config::blocks_per_round> native_contract_chain_initializer::get_chain_start_producers() {
   std::array<types::account_name, config::blocks_per_round> result;
   std::transform(genesis.initial_producers.begin(), genesis.initial_producers.end(), result.begin(),
                  [](const auto& p) { return p.owner_name; });
   return result;
}

void native_contract_chain_initializer::register_types(chain_controller& chain, chainbase::database& db) {
   // Install the native contract's indexes; we can't do anything until our objects are recognized
   db.add_index<eosio::chain::staked_balance_multi_index>();
   db.add_index<eosio::chain::producer_votes_multi_index>();
   db.add_index<eosio::chain::proxy_vote_multi_index>();
   db.add_index<eosio::chain::producer_schedule_multi_index>();

   db.add_index<eosio::chain::balance_multi_index>();

#define SET_APP_HANDLER( contract, scope, action, nspace ) \
   chain.set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(native::nspace::apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )
   SET_APP_HANDLER( eos, eos, newaccount, eosio );
   SET_APP_HANDLER( eos, eos, transfer, eosio );
   SET_APP_HANDLER( eos, eos, lock, eosio );
   SET_APP_HANDLER( eos, eos, claim, eosio );
   SET_APP_HANDLER( eos, eos, unlock, eosio );
   SET_APP_HANDLER( eos, eos, okproducer, eosio );
   SET_APP_HANDLER( eos, eos, setproducer, eosio );
   SET_APP_HANDLER( eos, eos, setproxy, eosio );
   SET_APP_HANDLER( eos, eos, setcode, eosio );
   SET_APP_HANDLER( eos, eos, updateauth, eosio );
   SET_APP_HANDLER( eos, eos, deleteauth, eosio );
   SET_APP_HANDLER( eos, eos, linkauth, eosio );
   SET_APP_HANDLER( eos, eos, unlinkauth, eosio ); 
}

types::abi native_contract_chain_initializer::eos_contract_abi()
{
   types::abi eos_abi;
//   eos_abi.types.push_back( types::type_def{"account_name","name"} );
   eos_abi.types.push_back( types::type_def{"share_type","int64"} );
   eos_abi.actions.push_back( types::action{name("transfer"), "transfer"} );
   eos_abi.actions.push_back( types::action{name("nonce"), "nonce"} );
   eos_abi.actions.push_back( types::action{name("lock"), "lock"} );
   eos_abi.actions.push_back( types::action{name("unlock"), "unlock"} );
   eos_abi.actions.push_back( types::action{name("claim"), "claim"} );
   eos_abi.actions.push_back( types::action{name("okproducer"), "okproducer"} );
   eos_abi.actions.push_back( types::action{name("setproducer"), "setproducer"} );
   eos_abi.actions.push_back( types::action{name("setproxy"), "setproxy"} );
   eos_abi.actions.push_back( types::action{name("setcode"), "setcode"} );
   eos_abi.actions.push_back( types::action{name("linkauth"), "linkauth"} );
   eos_abi.actions.push_back( types::action{name("unlinkauth"), "unlinkauth"} );
   eos_abi.actions.push_back( types::action{name("updateauth"), "updateauth"} );
   eos_abi.actions.push_back( types::action{name("deleteauth"), "deleteauth"} );
   eos_abi.actions.push_back( types::action{name("newaccount"), "newaccount"} );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::transfer>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::nonce>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::lock>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::unlock>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::claim>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::okproducer>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::setproducer>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::setproxy>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::setcode>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::updateauth>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::linkauth>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::unlinkauth>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::deleteauth>::type() );
   eos_abi.structs.push_back( eosio::types::get_struct<eosio::types::newaccount>::type() );

   return eos_abi;
}

std::vector<message> native_contract_chain_initializer::prepare_database(chain_controller& chain,
                                                                                chainbase::database& db) {
   std::vector<message> messages_to_process;

   // Create the singleton object, producer_schedule_object
   db.create<eosio::chain::producer_schedule_object>([](const auto&){});

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto CreateNativeAccount = [this, &db](name name, auto liquidBalance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;

         if( name == config::eos_contract_name ) {
            a.set_abi(eos_contract_abi());
         }
      });
      const auto& owner = db.create<permission_object>([&name](permission_object& p) {
         p.owner = name;
         p.name = "owner";
         p.auth.threshold = 1;
      });
      db.create<permission_object>([&name, &owner](permission_object& p) {
         p.owner = name;
         p.parent = owner.id;
         p.name = "active";
         p.auth.threshold = 1;
      });
      db.create<eosio::chain::balance_object>([&name, liquidBalance]( auto& b) {
         b.owner_name = name;
         b.balance = liquidBalance;
      });
      db.create<eosio::chain::staked_balance_object>([&name](auto& sb) { sb.ownerName = name; });
   };
   CreateNativeAccount(config::eos_contract_name, config::initial_token_supply);

   // Queue up messages which will run contracts to create the initial accounts
   auto KeyAuthority = [](public_key k) {
      return types::authority(1, {{k, 1}}, {});
   };
   for (const auto& acct : genesis.initial_accounts) {
      message msg(config::eos_contract_name,
                             vector<types::account_permission>{{config::eos_contract_name, "active"}},
                             "newaccount", types::newaccount(config::eos_contract_name, acct.name,
                                                             KeyAuthority(acct.owner_key),
                                                             KeyAuthority(acct.active_key),
                                                             KeyAuthority(acct.owner_key),
                                                             acct.staking_balance));
      messages_to_process.emplace_back(std::move(msg));
      if (acct.liquid_balance > 0) {
         msg = message(config::eos_contract_name,
                                  vector<types::account_permission>{{config::eos_contract_name, "active"}},
                                  "transfer", types::transfer(config::eos_contract_name, acct.name,
                                                              acct.liquid_balance.amount, "Genesis Allocation"));
         messages_to_process.emplace_back(std::move(msg));
      }
   }

   // Create initial producers
   auto CreateProducer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return message(config::eos_contract_name, vector<types::account_permission>{{p.owner_name, "active"}},
                            "setproducer", types::setproducer(p.owner_name, p.block_signing_key, config));
   });
   boost::copy(genesis.initial_producers | CreateProducer, std::back_inserter(messages_to_process));

   // Create special accounts
   auto create_special_account = [this, &db](name name, const auto& owner, const auto& active) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      const auto& owner_permission = db.create<permission_object>([&owner, &name](permission_object& p) {
         p.name = config::owner_name;
         p.parent = 0;
         p.owner = name;
         p.auth = std::move(owner);
      });
      db.create<permission_object>([&active, &owner_permission](permission_object& p) {
         p.name = config::active_name;
         p.parent = owner_permission.id;
         p.owner = owner_permission.owner;
         p.auth = std::move(active);
      });
   };

   auto empty_authority = types::authority(0, {}, {});
   auto active_producers_authority = types::authority(config::producers_authority_threshold, {}, {});
   for(auto& p : genesis.initial_producers) {
      active_producers_authority.accounts.push_back({{p.owner_name, config::active_name}, 1});
   }

   //CreateNativeAccount(config::anybody_account_name, 0);
   create_special_account(config::nobody_account_name, empty_authority, empty_authority);
   create_special_account(config::producers_account_name, empty_authority, active_producers_authority);

   return messages_to_process;
}

} } // namespace eosio::native_contract
