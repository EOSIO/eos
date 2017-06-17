#include <eos/native_contract/system_contract.hpp>

#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/type_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/global_property_object.hpp>

#include <eos/chain/wasm_interface.hpp>

namespace eos {
using namespace chain;

void DefineStruct::validate(message_validate_context& context) {
   auto  msg = context.msg.as<types::DefineStruct>();
   EOS_ASSERT(msg.definition.name != TypeName(), message_validate_exception, "must define a type name");
   // TODO:  validate_type_name( msg.definition.name)
   //   validate_type_name( msg.definition.base)
}

void DefineStruct::validate_preconditions(precondition_validate_context& context) {
   auto& db = context.db;
   auto  msg = context.msg.as<types::DefineStruct>();
   db.get<account_object,by_name>(msg.scope);
#warning TODO:  db.get<account_object>(sg.base_scope)
}

void DefineStruct::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto  msg = context.msg.as<types::DefineStruct>();

   db.create<type_object>( [&](auto& type) {
      type.scope = msg.scope;
      type.name  = msg.definition.name;
      type.fields.reserve(msg.definition.fields.size());
#warning TODO:  type.base_scope =
      type.base  = msg.definition.base;
      for(const auto& f : msg.definition.fields) {
         type.fields.push_back(f);
      }
   });
}

void SetCode::validate(message_validate_context& context) {
   auto  msg = context.msg.as<types::SetCode>();
   FC_ASSERT( msg.vmtype == 0 );
   FC_ASSERT( msg.vmversion == 0 );
   // TODO: verify code compiles and is properly sanitized
}

void SetCode::validate_preconditions(precondition_validate_context& context)
{ try {
   auto& db = context.db;
   auto  msg = context.msg.as<types::SetCode>();
   // db.get<type_object,by_scope_name>( boost::make_tuple(msg.account, msg.type))

} FC_CAPTURE_AND_RETHROW() }

void SetCode::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto  msg = context.msg.as<types::SetCode>();
   const auto& account = db.get<account_object,by_name>(msg.account);
   wlog( "set code: ${size}", ("size",msg.code.size()));
   db.modify( account, [&]( auto& a ) {
      a.code_version++;
      a.code.resize( msg.code.size() );
      memcpy( a.code.data(), msg.code.data(), msg.code.size() );
   });

   apply_context init_context( context.mutable_db, chain::Message(), msg.account );
   wasm_interface::get().init( init_context );
}

void CreateAccount::validate(message_validate_context& context) {
   auto create = context.msg.as<types::CreateAccount>();

   EOS_ASSERT(context.msg.has_notify(config::EosContractName), message_validate_exception,
              "Must notify EOS Contract (${name})", ("name", config::EosContractName));
   EOS_ASSERT(context.msg.has_notify(config::StakedBalanceContractName), message_validate_exception,
              "Must notify Staked Balance Contract (${name})", ("name", config::StakedBalanceContractName));

   EOS_ASSERT( eos::validate(create.owner), message_validate_exception, "Invalid owner authority");
   EOS_ASSERT( eos::validate(create.active), message_validate_exception, "Invalid active authority");
   EOS_ASSERT( eos::validate(create.recovery), message_validate_exception, "Invalid recovery authority");
}

void CreateAccount::validate_preconditions(precondition_validate_context& context) {
   const auto& db = context.db;
   auto create = context.msg.as<types::CreateAccount>();

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

void CreateAccount::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto create = context.msg.as<types::CreateAccount>();
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

} // namespace eos
