#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/objects.hpp>
#include <eos/native_contract/eos_contract.hpp>

#include <eos/chain/producer_object.hpp>
#include <eos/chain/permission_object.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace eos { namespace native_contract {
using namespace chain;
namespace chain = ::eos::chain;

types::Time native_contract_chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

chain::BlockchainConfiguration native_contract_chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

std::array<types::AccountName, config::BlocksPerRound> native_contract_chain_initializer::get_chain_start_producers() {
   std::array<types::AccountName, config::BlocksPerRound> result;
   std::transform(genesis.initial_producers.begin(), genesis.initial_producers.end(), result.begin(),
                  [](const auto& p) { return p.owner_name; });
   return result;
}

void native_contract_chain_initializer::register_types(chain_controller& chain, chainbase::database& db) {
   // Install the native contract's indexes; we can't do anything until our objects are recognized
   db.add_index<native::eos::StakedBalanceMultiIndex>();
   db.add_index<native::eos::ProducerVotesMultiIndex>();
   db.add_index<native::eos::ProxyVoteMultiIndex>();
   db.add_index<native::eos::ProducerScheduleMultiIndex>();

   db.add_index<native::eos::BalanceMultiIndex>();

#define SET_APP_HANDLER( contract, scope, action ) \
   chain.set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(native::contract::apply_, BOOST_PP_CAT(scope, BOOST_PP_CAT(_,action) ) ) )

   SET_APP_HANDLER( eos, eos, newaccount );
   SET_APP_HANDLER( eos, eos, transfer );
   SET_APP_HANDLER( eos, eos, lock );
   SET_APP_HANDLER( eos, eos, claim );
   SET_APP_HANDLER( eos, eos, unlock );
   SET_APP_HANDLER( eos, eos, okproducer );
   SET_APP_HANDLER( eos, eos, setproducer );
   SET_APP_HANDLER( eos, eos, setproxy );
   SET_APP_HANDLER( eos, eos, setcode );
}

std::vector<chain::Message> native_contract_chain_initializer::prepare_database(chain_controller& chain,
                                                                                chainbase::database& db) {
   std::vector<chain::Message> messages_to_process;

   // Create the singleton object, ProducerScheduleObject
   db.create<native::eos::ProducerScheduleObject>([](const auto&){});

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto CreateNativeAccount = [this, &db](Name name, auto liquidBalance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;

         if( name == config::EosContractName ) {
            types::Abi eos_abi;
            eos_abi.types.push_back( types::TypeDef{"AccountName","Name"} );
            eos_abi.actions.push_back( types::Action{Name("transfer"), "transfer"} );
            eos_abi.structs.push_back( eos::types::GetStruct<eos::types::transfer>::type() );
            a.set_abi(eos_abi);
         }
      });
      db.create<native::eos::BalanceObject>([&name, liquidBalance]( auto& b) {
         b.ownerName = name;
         b.balance = liquidBalance;
      });
      db.create<native::eos::StakedBalanceObject>([&name](auto& sb) { sb.ownerName = name; });
   };
   CreateNativeAccount(config::EosContractName, config::InitialTokenSupply);

   // Queue up messages which will run contracts to create the initial accounts
   auto KeyAuthority = [](PublicKey k) {
      return types::Authority(1, {{k, 1}}, {});
   };
   for (const auto& acct : genesis.initial_accounts) {
      chain::Message message(config::EosContractName,
                             "newaccount", types::newaccount(config::EosContractName, acct.name,
                                                             KeyAuthority(acct.owner_key),
                                                             KeyAuthority(acct.active_key),
                                                             KeyAuthority(acct.owner_key),
                                                             acct.staking_balance));
      messages_to_process.emplace_back(std::move(message));
      if (acct.liquid_balance > 0) {
         message = chain::Message(config::EosContractName, 
                                  "transfer", types::transfer(config::EosContractName, acct.name,
                                                              acct.liquid_balance.amount/*, "Genesis Allocation"*/));
         messages_to_process.emplace_back(std::move(message));
      }
   }

   // Create initial producers
   auto CreateProducer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return chain::Message(config::EosContractName, 
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
