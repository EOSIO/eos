#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/objects.hpp>
#include <eos/native_contract/staked_balance_contract.hpp>
#include <eos/native_contract/eos_contract.hpp>
#include <eos/native_contract/system_contract.hpp>

#include <eos/chain/producer_object.hpp>

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
   db.add_index<native::staked::StakedBalanceMultiIndex>();
   db.add_index<native::staked::ProducerVotesMultiIndex>();
   db.add_index<native::staked::ProxyVoteMultiIndex>();
   db.add_index<native::staked::ProducerScheduleMultiIndex>();

   db.add_index<native::eos::BalanceMultiIndex>();

#define SET_PRE_HANDLER( contract, scope, action ) \
   chain.set_precondition_validate_handler( Name(#contract), Name(#scope), Name(#action), BOOST_PP_CAT(&native::contract::precondition_, BOOST_PP_CAT( BOOST_PP_CAT(scope, _), action)) )
#define SET_VAL_HANDLER( contract, scope, action ) \
   chain.set_validate_handler( #contract, #scope, #action, &BOOST_PP_CAT(native::contract::validate_, BOOST_PP_CAT(scope, BOOST_PP_CAT(_,action) ) ) )
#define SET_APP_HANDLER( contract, scope, action ) \
   chain.set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(native::contract::apply_, BOOST_PP_CAT(scope, BOOST_PP_CAT(_,action) ) ) )

   SET_PRE_HANDLER( eos, system, newaccount );
   SET_APP_HANDLER( eos, system, newaccount );

   SET_APP_HANDLER( eos, staked, claim );

   SET_VAL_HANDLER( eos, eos, transfer );
   SET_PRE_HANDLER( eos, eos, transfer );
   SET_APP_HANDLER( eos, eos, transfer );

   SET_VAL_HANDLER( eos, eos, lock );
   SET_PRE_HANDLER( eos, eos, lock );
   SET_APP_HANDLER( eos, eos, lock );

   SET_APP_HANDLER( staked, system, newaccount );
   SET_APP_HANDLER( staked, eos, lock );

   SET_VAL_HANDLER( staked, staked, claim );
   SET_PRE_HANDLER( staked, staked, claim );
   SET_APP_HANDLER( staked, staked, claim );

   SET_VAL_HANDLER( staked, staked, unlock );
   SET_PRE_HANDLER( staked, staked, unlock );
   SET_APP_HANDLER( staked, staked, unlock );

   SET_VAL_HANDLER( staked, staked, okproducer );
   SET_PRE_HANDLER( staked, staked, okproducer );
   SET_APP_HANDLER( staked, staked, okproducer );

   SET_VAL_HANDLER( staked, staked, setproducer );
   SET_PRE_HANDLER( staked, staked, setproducer );
   SET_APP_HANDLER( staked, staked, setproducer );

   SET_VAL_HANDLER( staked, staked, setproxy );
   SET_PRE_HANDLER( staked, staked, setproxy );
   SET_APP_HANDLER( staked, staked, setproxy );

   SET_VAL_HANDLER( system, system, setcode );
   SET_PRE_HANDLER( system, system, setcode );
   SET_APP_HANDLER( system, system, setcode );

   SET_VAL_HANDLER( system, system, newaccount );
   SET_PRE_HANDLER( system, system, newaccount );
   SET_APP_HANDLER( system, system, newaccount );
}

std::vector<chain::Message> native_contract_chain_initializer::prepare_database(chain_controller& chain,
                                                                                chainbase::database& db) {
   std::vector<chain::Message> messages_to_process;

   // Create the singleton object, ProducerScheduleObject
   db.create<native::staked::ProducerScheduleObject>([](const auto&){});

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto CreateNativeAccount = [this, &db](Name name, auto liquidBalance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      db.create<native::eos::BalanceObject>([&name, liquidBalance]( auto& b) {
         b.ownerName = name;
         b.balance = liquidBalance;
      });
      db.create<native::staked::StakedBalanceObject>([&name](auto& sb) { sb.ownerName = name; });
   };
   CreateNativeAccount(config::SystemContractName, config::InitialTokenSupply);
   CreateNativeAccount(config::EosContractName, 0);
   CreateNativeAccount(config::StakedBalanceContractName, 0);

   // Queue up messages which will run contracts to create the initial accounts
   auto KeyAuthority = [](PublicKey k) {
      return types::Authority(1, {{k, 1}}, {});
   };
   for (const auto& acct : genesis.initial_accounts) {
      chain::Message message(config::SystemContractName,
                             vector<AccountName>{config::EosContractName, config::StakedBalanceContractName},
                             vector<types::AccountPermission>{},
                             "newaccount", types::newaccount(config::SystemContractName, acct.name,
                                                             KeyAuthority(acct.owner_key),
                                                             KeyAuthority(acct.active_key),
                                                             KeyAuthority(acct.owner_key),
                                                             acct.staking_balance));
      messages_to_process.emplace_back(std::move(message));
      if (acct.liquid_balance > 0) {
         message = chain::Message(config::EosContractName, vector<AccountName>{config::SystemContractName, acct.name},
                                  vector<types::AccountPermission>{},
                                  "transfer", types::transfer(config::SystemContractName, acct.name,
                                                              acct.liquid_balance, "Genesis Allocation"));
         messages_to_process.emplace_back(std::move(message));
      }
   }

   // Create initial producers
   auto CreateProducer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return chain::Message(config::StakedBalanceContractName, {p.owner_name},
                            vector<types::AccountPermission>{},
                            "setproducer", types::setproducer(p.owner_name, p.block_signing_key, config));
   });
   boost::copy(genesis.initial_producers | CreateProducer, std::back_inserter(messages_to_process));

   return messages_to_process;
}

} } // namespace eos::native_contract
