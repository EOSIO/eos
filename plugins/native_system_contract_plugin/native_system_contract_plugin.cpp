#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/chain/action_objects.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>

#include <boost/preprocessor/seq/for_each.hpp>

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

void CreateProducer_validate(chain::message_validate_context& context) {
   auto create = context.msg.as<CreateProducer>();
   EOS_ASSERT(create.name.size() > 0, message_validate_exception, "Producer owner name cannot be empty");
   EOS_ASSERT(create.key != PublicKey(), message_validate_exception, "Producer signing key cannot be null");
}
void CreateProducer_validate_preconditions(chain::precondition_validate_context& context) {
   auto create = context.msg.as<CreateProducer>();
   const auto& db = context.db;
   const auto& owner = db.get_account(create.name);
   auto producer = db.find<producer_object, by_owner>(owner.id);
   EOS_ASSERT(producer == nullptr, message_precondition_exception,
              "Account ${name} already has a block producer", ("name", create.name));
}
void CreateProducer_apply(chain::apply_context& context) {
   auto create = context.msg.as<CreateProducer>();
   auto& db = context.mutable_db;
   const auto& owner = db.get_account(create.name);
   db.create<producer_object>([&create, &owner](producer_object& p) {
      p.owner = owner.id;
      p.signing_key = create.key;
   });
}

#include "system_contract_impl.hpp"
void native_system_contract_plugin::install(database& db) {
#define SET_HANDLERS(name) \
   db.set_validate_handler("sys", "sys", #name, &name ## _validate); \
   db.set_precondition_validate_handler("sys", "sys", #name, &name ## _validate_preconditions); \
   db.set_apply_handler("sys", "sys", #name, &name ## _apply);
#define FWD_SET_HANDLERS(r, data, elem) SET_HANDLERS(elem)

   BOOST_PP_SEQ_FOR_EACH(FWD_SET_HANDLERS, x, EOS_SYSTEM_CONTRACT_FUNCTIONS)
}
}
