/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/contract/native_contract_chain_initializer.hpp>
#include <eos/chain/contract/objects.hpp>
#include <eos/chain/contract/eos_contract.hpp>

#include <eos/chain/producer_object.hpp>
#include <eos/chain/permission_object.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace eosio { namespace native_contract {
using namespace eosio::chain;

types::Time native_contract_chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

blockchain_configuration native_contract_chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

std::array<types::account_name, config::BlocksPerRound> native_contract_chain_initializer::get_chain_start_producers() {
   std::array<types::account_name, config::BlocksPerRound> result;
   std::transform(genesis.initial_producers.begin(), genesis.initial_producers.end(), result.begin(),
                  [](const auto& p) { return p.owner_name; });
   return result;
}

void native_contract_chain_initializer::register_types(chain_controller& chain, chainbase::database& db) {
   // Install the native contract's indexes; we can't do anything until our objects are recognized
   db.add_index<eosio::chain::contracts::staked_balance_multi_index>();
   db.add_index<eosio::chain::contracts::producer_votes_multi_index>();
   db.add_index<eosio::chain::contracts::proxy_vote_multi_index>();
   db.add_index<eosio::chain::contracts::producer_schedule_multi_index>();

   db.add_index<eosio::chain::contracts::balance_multi_index>();

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
   eos_abi.types.push_back( types::TypeDef{"AccountName","Name"} );
   eos_abi.types.push_back( types::TypeDef{"ShareType","Int64"} );
   eos_abi.actions.push_back( types::Action{Name("transfer"), "transfer"} );
   eos_abi.actions.push_back( types::Action{Name("lock"), "lock"} );
   eos_abi.actions.push_back( types::Action{Name("unlock"), "unlock"} );
   eos_abi.actions.push_back( types::Action{Name("claim"), "claim"} );
   eos_abi.actions.push_back( types::Action{Name("okproducer"), "okproducer"} );
   eos_abi.actions.push_back( types::Action{Name("setproducer"), "setproducer"} );
   eos_abi.actions.push_back( types::Action{Name("setproxy"), "setproxy"} );
   eos_abi.actions.push_back( types::Action{Name("setcode"), "setcode"} );
   eos_abi.actions.push_back( types::Action{Name("linkauth"), "linkauth"} );
   eos_abi.actions.push_back( types::Action{Name("unlinkauth"), "unlinkauth"} );
   eos_abi.actions.push_back( types::Action{Name("updateauth"), "updateauth"} );
   eos_abi.actions.push_back( types::Action{Name("deleteauth"), "deleteauth"} );
   eos_abi.actions.push_back( types::Action{Name("newaccount"), "newaccount"} );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::transfer>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::lock>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::unlock>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::claim>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::okproducer>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::setproducer>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::setproxy>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::setcode>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::updateauth>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::linkauth>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::unlinkauth>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::deleteauth>::type() );
   eos_abi.structs.push_back( eosio::types::GetStruct<eosio::types::newaccount>::type() );

   return eos_abi;
}

std::vector<Message> native_contract_chain_initializer::prepare_database(chain_controller& chain,
                                                                                chainbase::database& db) {
   std::vector<Message> messages_to_process;

   // Create the singleton object, producer_schedule_object
   db.create<eosio::chain::contracts::producer_schedule_object>([](const auto&){});

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto CreateNativeAccount = [this, &db](Name name, auto liquidBalance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;

         if( name == config::EosContractName ) {
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
      db.create<eosio::chain::contracts::balance_object>([&name, liquidBalance]( auto& b) {
         b.owner_name = name;
         b.balance = liquidBalance;
      });
      db.create<eosio::chain::contracts::staked_balance_object>([&name](auto& sb) { sb.owner_name = name; });
   };
   CreateNativeAccount(config::EosContractName, config::InitialTokenSupply);

   // Queue up messages which will run contracts to create the initial accounts
   auto KeyAuthority = [](public_key k) {
      return types::Authority(1, {{k, 1}}, {});
   };
   for (const auto& acct : genesis.initial_accounts) {
      Message message(config::EosContractName,
                             vector<types::AccountPermission>{{config::EosContractName, "active"}},
                             "newaccount", types::newaccount(config::EosContractName, acct.name,
                                                             KeyAuthority(acct.owner_key),
                                                             KeyAuthority(acct.active_key),
                                                             KeyAuthority(acct.owner_key),
                                                             acct.staking_balance));
      messages_to_process.emplace_back(std::move(message));
      if (acct.liquid_balance > 0) {
         message = Message(config::EosContractName,
                                  vector<types::AccountPermission>{{config::EosContractName, "active"}},
                                  "transfer", types::transfer(config::EosContractName, acct.name,
                                                              acct.liquid_balance.amount, "Genesis Allocation"));
         messages_to_process.emplace_back(std::move(message));
      }
   }

   // Create initial producers
   auto CreateProducer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return Message(config::EosContractName, vector<types::AccountPermission>{{p.owner_name, "active"}},
                            "setproducer", types::setproducer(p.owner_name, p.block_signing_key, config));
   });
   boost::copy(genesis.initial_producers | CreateProducer, std::back_inserter(messages_to_process));

   // Create special accounts
   auto create_special_account = [this, &db](Name name, const auto& owner, const auto& active) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      const auto& owner_permission = db.create<permission_object>([&owner, &name](permission_object& p) {
         p.name = config::OwnerName;
         p.parent = 0;
         p.owner = name;
         p.auth = std::move(owner);
      });
      db.create<permission_object>([&active, &owner_permission](permission_object& p) {
         p.name = config::ActiveName;
         p.parent = owner_permission.id;
         p.owner = owner_permission.owner;
         p.auth = std::move(active);
      });
   };

   auto empty_authority = types::Authority(0, {}, {});
   auto active_producers_authority = types::Authority(config::ProducersAuthorityThreshold, {}, {});
   for(auto& p : genesis.initial_producers) {
      active_producers_authority.accounts.push_back({{p.owner_name, config::ActiveName}, 1});
   }

   //CreateNativeAccount(config::AnybodyAccountName, 0);
   create_special_account(config::NobodyAccountName, empty_authority, empty_authority);
   create_special_account(config::ProducersAccountName, empty_authority, active_producers_authority);

   return messages_to_process;
}

} } // namespace eos::native_contract
