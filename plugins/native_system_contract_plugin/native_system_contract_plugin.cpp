#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/native_system_contract_plugin/staked_balance_contract.hpp>
#include <eos/native_system_contract_plugin/system_contract.hpp>
#include <eos/native_system_contract_plugin/eos_contract.hpp>

#include <eos/chain/action_objects.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>

#include <boost/preprocessor/seq/for_each.hpp>

namespace eos {

class native_system_contract_plugin_impl {
public:
   native_system_contract_plugin_impl(chain_controller& chain)
      : chain(chain) {}

   chain_controller& chain;
};

native_system_contract_plugin::native_system_contract_plugin()
   : my(new native_system_contract_plugin_impl(app().get_plugin<chain_plugin>().chain())){}
native_system_contract_plugin::~native_system_contract_plugin(){}

void native_system_contract_plugin::plugin_initialize(const variables_map&) {
   install(my->chain);
}

void native_system_contract_plugin::plugin_startup() {
}

void native_system_contract_plugin::plugin_shutdown() {
}

void native_system_contract_plugin::install(chain_controller& db) {
#define SET_HANDLERS(contractname, handlername) \
   db.set_validate_handler(contractname, contractname, #handlername, &handlername::validate); \
   db.set_precondition_validate_handler(contractname, contractname, #handlername, &handlername::validate_preconditions); \
   db.set_apply_handler(contractname, contractname, #handlername, &handlername::apply);
#define FWD_SET_HANDLERS(r, data, elem) SET_HANDLERS(data, elem)

   // Set message handlers
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::SystemContractName, EOS_SYSTEM_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::EosContractName, EOS_CONTRACT_FUNCTIONS)
   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, config::StakedBalanceContractName, EOS_STAKED_BALANCE_CONTRACT_FUNCTIONS)

   // Set notify handlers
   db.set_apply_handler(config::EosContractName, config::StakedBalanceContractName,
                        "TransferToLocked", &TransferToLocked_Notify_Staked);
   db.set_apply_handler(config::StakedBalanceContractName, config::EosContractName,
                        "ClaimUnlockedEos", &ClaimUnlockedEos_Notify_Eos);
#warning TODO: Notify eos and sbc when an account is created
}

}
