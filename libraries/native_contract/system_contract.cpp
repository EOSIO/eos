#include <eos/native_contract/system_contract.hpp>

#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/global_property_object.hpp>

#include <eos/chain/wasm_interface.hpp>

namespace native {
namespace system {
using namespace chain;
namespace config = ::eos::config;

void validate_system_setcode(message_validate_context& context) {
   auto  msg = context.msg.as<types::setcode>();
   FC_ASSERT( msg.vmtype == 0 );
   FC_ASSERT( msg.vmversion == 0 );
#warning TODO: verify code compiles and is properly sanitized
}

void precondition_system_setcode(precondition_validate_context& context)
{ try {
   //auto& db = context.db;
   //auto  msg = context.msg.as<types::setcode>();
} FC_CAPTURE_AND_RETHROW() }

void apply_system_setcode(apply_context& context) {
   auto& db = context.mutable_db;
   auto  msg = context.msg.as<types::setcode>();
   const auto& account = db.get<account_object,by_name>(msg.account);
   wlog( "set code: ${size}", ("size",msg.code.size()));
   db.modify( account, [&]( auto& a ) {
      /** TODO: consider whether a microsecond level local timestamp is sufficient */
      #warning TODO: update setcode message to include the hash, then validate it in validate 
      a.code_version = fc::sha256::hash( msg.code.data(), msg.code.size() );
      a.code.resize( msg.code.size() );
      memcpy( a.code.data(), msg.code.data(), msg.code.size() );
   });

   apply_context init_context( context.mutable_controller, context.mutable_db, context.trx, context.msg, msg.account );
   wasm_interface::get().init( init_context );
}

void validate_system_newaccount(message_validate_context& context) {
   auto create = context.msg.as<types::newaccount>();

   EOS_ASSERT(context.msg.has_notify(config::EosContractName), message_validate_exception,
              "Must notify EOS Contract (${name})", ("name", config::EosContractName));
   EOS_ASSERT(context.msg.has_notify(config::StakedBalanceContractName), message_validate_exception,
              "Must notify Staked Balance Contract (${name})", ("name", config::StakedBalanceContractName));

   EOS_ASSERT( validate(create.owner), message_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), message_validate_exception, "Invalid active authority");
   EOS_ASSERT( validate(create.recovery), message_validate_exception, "Invalid recovery authority");
}

void precondition_system_newaccount(precondition_validate_context& context) {
   const auto& db = context.db;
   auto create = context.msg.as<types::newaccount>();

   db.get<account_object,by_name>(create.creator); ///< make sure it exists

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, message_precondition_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

#warning TODO: make sure creation deposit is greater than min account balance

   auto validate_authority_preconditions = [&context](const auto& auth) {
      for(const auto& a : auth.accounts)
         context.db.get<account_object,by_name>(a.permission.account);
   };
   validate_authority_preconditions(create.owner);
   validate_authority_preconditions(create.active);
   validate_authority_preconditions(create.recovery);
}

void apply_system_newaccount(apply_context& context) {
   auto& db = context.mutable_db;
   auto create = context.msg.as<types::newaccount>();
   const auto& new_account = db.create<account_object>([&create, &db](account_object& a) {
      a.name = create.name;
      a.creation_date = db.get(dynamic_global_property_object::id_type()).time;
   });
   const auto& owner_permission = db.create<permission_object>([&create, &new_account](permission_object& p) {
      p.name = "owner";
      p.parent = 0;
      p.owner = new_account.name;
      p.auth = std::move(create.owner);
   });
   db.create<permission_object>([&create, &owner_permission](permission_object& p) {
      p.name = "active";
      p.parent = owner_permission.id;
      p.owner = owner_permission.owner;
      p.auth = std::move(create.active);
   });
}

} } // namespace native::system
