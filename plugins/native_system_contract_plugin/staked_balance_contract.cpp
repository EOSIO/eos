#include <eos/native_system_contract_plugin/staked_balance_contract.hpp>

#include <eos/chain/producer_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace eos {
using namespace chain;

void CreateProducer::validate(message_validate_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   EOS_ASSERT(create.name.size() > 0, message_validate_exception, "Producer owner name cannot be empty");
}

void CreateProducer::validate_preconditions(precondition_validate_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   const auto& db = context.db;
   const auto& owner = db.get<account_object,by_name>(create.name);
   auto producer = db.find<producer_object, by_owner>(owner.id);
   EOS_ASSERT(producer == nullptr, message_precondition_exception,
              "Account ${name} already has a block producer", ("name", create.name));
}

void CreateProducer::apply(apply_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   auto& db = context.mutable_db;
   const auto& owner = db.get<account_object,by_name>(create.name);
   db.create<producer_object>([&create, &owner](producer_object& p) {
      p.owner = owner.id;
      p.signing_key = create.key;
   });
}

void UpdateProducer::validate(message_validate_context& context) {
   auto update = context.msg.as<types::UpdateProducer>();
   EOS_ASSERT(update.name.size() > 0, message_validate_exception, "Producer owner name cannot be empty");
}

void UpdateProducer::validate_preconditions(precondition_validate_context& context) {
   const auto& db = context.db;
   auto update = context.msg.as<types::UpdateProducer>();
   const auto& producer = db.get<producer_object, by_owner>(db.get<account_object, by_name>(update.name).id);
   EOS_ASSERT(producer.signing_key != update.newKey || producer.configuration != update.configuration,
              message_validate_exception, "Producer's new settings may not be identical to old settings");
}

void UpdateProducer::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto update = context.msg.as<types::UpdateProducer>();
   const auto& producer = db.get<producer_object, by_owner>(db.get<account_object, by_name>(update.name).id);

   db.modify(producer, [&update](producer_object& p) {
      p.signing_key = update.newKey;
      p.configuration = update.configuration;
   });
}

} // namespace eos
