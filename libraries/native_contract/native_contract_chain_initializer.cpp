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

std::vector<chain::Message> native_contract_chain_initializer::prepare_database(chain_controller& chain,
                                                                                chainbase::database& db) {
   std::vector<chain::Message> messages_to_process;

   // Install the native contract's indexes; we can't do anything until our objects are recognized
   db.add_index<StakedBalanceMultiIndex>();
   db.add_index<BalanceMultiIndex>();
   db.add_index<ProducerVotesMultiIndex>();

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto CreateNativeAccount = [this, &db](auto name, auto liquidBalance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      db.create<BalanceObject>([&name, liquidBalance](BalanceObject& b) {
         b.ownerName = name;
         b.balance = liquidBalance;
      });
      db.create<StakedBalanceObject>([&name](StakedBalanceObject& sb) { sb.ownerName = name; });
   };
   CreateNativeAccount(config::SystemContractName, config::InitialTokenSupply);
   CreateNativeAccount(config::EosContractName, 0);
   CreateNativeAccount(config::StakedBalanceContractName, 0);

   // Install the native contract's message handlers
   // First, set message handlers
#define SET_HANDLERS(contractname, handlername) \
   chain.set_validate_handler(contractname, contractname, #handlername, &handlername::validate); \
   chain.set_precondition_validate_handler(contractname, contractname, #handlername, &handlername::validate_preconditions); \
   chain.set_apply_handler(contractname, contractname, #handlername, &handlername::apply);
#define FWD_SET_HANDLERS(r, data, elem) SET_HANDLERS(data, elem)
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::SystemContractName, EOS_SYSTEM_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::EosContractName, EOS_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::StakedBalanceContractName, EOS_STAKED_BALANCE_CONTRACT_FUNCTIONS)
#undef FWD_SET_HANDLERS
#undef SET_HANDLERS

   // Second, set notify handlers
   auto SetNotifyHandlers = [&chain](auto recipient, auto scope, auto message, auto validate, auto apply) {
      chain.set_precondition_validate_handler(recipient, scope, message, validate);
      chain.set_apply_handler(recipient, scope, message, apply);
   };
   SetNotifyHandlers(config::EosContractName, config::StakedBalanceContractName, "TransferToLocked",
                     &TransferToLocked_Notify_Staked::validate_preconditions, &TransferToLocked_Notify_Staked::apply);
   SetNotifyHandlers(config::StakedBalanceContractName, config::EosContractName, "ClaimUnlockedEos",
                     &ClaimUnlockedEos_Notify_Eos::validate_preconditions, &ClaimUnlockedEos_Notify_Eos::apply);
   SetNotifyHandlers(config::SystemContractName, config::EosContractName, "CreateAccount",
                     &CreateAccount_Notify_Eos::validate_preconditions, &CreateAccount_Notify_Eos::apply);
   SetNotifyHandlers(config::SystemContractName, config::StakedBalanceContractName, "CreateAccount",
                     &CreateAccount_Notify_Staked::validate_preconditions, &CreateAccount_Notify_Staked::apply);

   // Register native contract message types
#define MACRO(r, data, elem) chain.register_type<types::elem>(data);
   BOOST_PP_SEQ_FOR_EACH(MACRO, config::SystemContractName, EOS_SYSTEM_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(MACRO, config::EosContractName, EOS_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(MACRO, config::StakedBalanceContractName, EOS_STAKED_BALANCE_CONTRACT_FUNCTIONS)
#undef MACRO

   // Queue up messages which will run contracts to create the initial accounts
   auto KeyAuthority = [](PublicKey k) {
      return types::Authority(1, {{k, 1}}, {});
   };
   for (const auto& acct : genesis.initial_accounts) {
      chain::Message message(config::SystemContractName, config::SystemContractName,
      {config::EosContractName, config::StakedBalanceContractName},
                             "CreateAccount", types::CreateAccount(config::SystemContractName, acct.name,
                                                                   KeyAuthority(acct.owner_key),
                                                                   KeyAuthority(acct.active_key),
                                                                   KeyAuthority(acct.owner_key),
                                                                   acct.staking_balance));
      messages_to_process.emplace_back(std::move(message));
      if (acct.liquid_balance > 0) {
         message = chain::Message(config::SystemContractName, config::EosContractName, {},
                                  "Transfer", types::Transfer(config::SystemContractName, acct.name,
                                                              acct.liquid_balance, "Genesis Allocation"));
         messages_to_process.emplace_back(std::move(message));
      }
   }

   // Create initial producers
   auto CreateProducer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return chain::Message(config::SystemContractName, config::StakedBalanceContractName, vector<AccountName>{},
                            "CreateProducer", types::CreateProducer(p.owner_name, p.block_signing_key, config));
   });
   boost::copy(genesis.initial_producers | CreateProducer, std::back_inserter(messages_to_process));

   return messages_to_process;
}

types::Time native_contract_chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

chain::BlockchainConfiguration native_contract_chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

std::array<types::AccountName, config::ProducerCount> native_contract_chain_initializer::get_chain_start_producers() {
   std::array<types::AccountName, config::ProducerCount> result;
   std::transform(genesis.initial_producers.begin(), genesis.initial_producers.end(), result.begin(),
                  [](const auto& p) { return p.owner_name; });
   return result;
}

} } // namespace eos::native_contract
