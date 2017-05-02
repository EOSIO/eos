#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/chain/action_objects.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>

namespace eos {
using namespace chain;
using namespace types;

class native_system_contract_plugin_impl {
public:
   native_system_contract_plugin_impl(database& db)
      : db(db) {}

   database& db;
};

native_system_contract_plugin::native_system_contract_plugin()
   : my(new native_system_contract_plugin_impl(app().get_plugin<chain_plugin>().db())){}
native_system_contract_plugin::~native_system_contract_plugin(){}

void native_system_contract_plugin::plugin_initialize(const variables_map&) {
   install(my->db);
}

void native_system_contract_plugin::plugin_startup() {
}

void native_system_contract_plugin::plugin_shutdown() {
}

#include "system_contract_impl.hpp"
void native_system_contract_plugin::install(database& db) {
#define SET_HANDLERS(name) \
   db.set_validate_handler("sys", "sys", #name, &name ## _validate); \
   db.set_precondition_validate_handler("sys", "sys", #name, &name ## _validate_preconditions); \
   db.set_apply_handler("sys", "sys", #name, &name ## _apply)

   SET_HANDLERS(Transfer);
   SET_HANDLERS(CreateAccount);
   SET_HANDLERS(DefineStruct);
   SET_HANDLERS(SetMessageHandler);
}
}
