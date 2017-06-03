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
#define SET_HANDLERS(name) \
   db.set_validate_handler("sys", "sys", #name, &name::validate); \
   db.set_precondition_validate_handler("sys", "sys", #name, &name::validate_preconditions); \
   db.set_apply_handler("sys", "sys", #name, &name::apply);
#define FWD_SET_HANDLERS(r, data, elem) SET_HANDLERS(elem)

   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, x, EOS_SYSTEM_CONTRACT_FUNCTIONS)
}

}
