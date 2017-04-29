#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>

namespace eos {
using namespace chain;

/************************************************************
 *
 *    Transfer Handlers
 *
 ***********************************************************/
void Transfer_validate(chain::message_validate_context& context) {
   auto transfer = context.msg.as<Transfer>();
   try {
      EOS_ASSERT(transfer.amount > Asset(0), message_validate_exception, "Must transfer a positive amount");
      EOS_ASSERT(context.msg.has_notify(transfer.to), message_validate_exception, "Must notify recipient of transfer");
   } FC_CAPTURE_AND_RETHROW( (transfer) ) 
}

void Transfer_validate_preconditions(chain::precondition_validate_context& context) {
   const auto& db = context.db;
   auto transfer = context.msg.as<Transfer>();

   db.get_account(transfer.to); ///< make sure this exists
   const auto& from = db.get_account(transfer.from);
   EOS_ASSERT(from.balance >= transfer.amount.amount, message_precondition_exception, "Insufficient Funds",
              ("from.balance",from.balance)("transfer.amount",transfer.amount));
}
void Transfer_apply(chain::apply_context& context) {
   auto& db = context.mutable_db;
   auto transfer = context.msg.as<Transfer>();
   idump((transfer));
   const auto& from = db.get_account(transfer.from);
   const auto& to = db.get_account(transfer.to);
   db.modify(from, [&](account_object& a) {
      a.balance -= transfer.amount;
   });
   db.modify(to, [&](account_object& a) {
      a.balance += transfer.amount;
   });
   idump((from));
   idump((to));
}


/************************************************************
 *
 *    Create Account Handlers
 *
 ***********************************************************/
///@{


void Authority_validate_preconditions( const Authority& auth, chain::precondition_validate_context& context ) {
   for( const auto& a : auth.accounts )
      context.db.get<account_object,by_name>( a.permission.account );
}

void CreateAccount_validate(chain::message_validate_context& context) {
   auto create = context.msg.as<CreateAccount>();

   EOS_ASSERT( validate(create.owner), message_validate_exception, "Invalid owner authority");
   EOS_ASSERT( validate(create.active), message_validate_exception, "Invalid active authority");
   EOS_ASSERT( validate(create.recovery), message_validate_exception, "Invalid recovery authority");
}

void CreateAccount_validate_preconditions(chain::precondition_validate_context& context) {
   const auto& db = context.db;
   auto create = context.msg.as<CreateAccount>();

   db.get_account(create.creator); ///< make sure it exists

   auto existing_account = db.find<account_object, by_name>(create.name);
   EOS_ASSERT(existing_account == nullptr, message_precondition_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.name));

   const auto& creator = db.get_account(context.msg.sender);
   EOS_ASSERT(creator.balance >= create.deposit, message_precondition_exception, "Insufficient Funds");

#warning TODO: make sure creation deposit is greater than min account balance

   Authority_validate_preconditions( create.owner, context );
   Authority_validate_preconditions( create.active, context );
   Authority_validate_preconditions( create.recovery, context );
}

void CreateAccount_apply(chain::apply_context& context) {
   auto& db = context.mutable_db;
   auto create = context.msg.as<CreateAccount>();
   db.modify(db.get_account(context.msg.sender), [&create](account_object& a) {
      a.balance -= create.deposit;
   });
   const auto& new_account = db.create<account_object>([&create](account_object& a) {
      a.name = create.name;
      a.balance = create.deposit;
   });
   const auto& owner_permission = db.create<permission_object>([&create, &new_account](permission_object& p) {
      p.name = "owner";
      p.parent = 0;
      p.owner = new_account.id;
      p.auth = std::move(create.owner);
   });
   db.create<permission_object>([&create, &owner_permission](permission_object& p) {
      p.name = "active";
      p.parent = owner_permission.id;
      p.owner = owner_permission.owner;
      p.auth = std::move(create.active);
   });
}
///@}  Create Account Handlers

class native_system_contract_plugin_impl {
public:
   native_system_contract_plugin_impl(database& db)
      : db(db) {}

   database& db;
};

native_system_contract_plugin::native_system_contract_plugin()
   : my(new native_system_contract_plugin_impl(app().get_plugin<chain_plugin>().db())){}
native_system_contract_plugin::~native_system_contract_plugin(){}

void native_system_contract_plugin::plugin_initialize(const variables_map& options) {
   install(my->db);
}

void native_system_contract_plugin::plugin_startup() {
   // Make the magic happen
}

void native_system_contract_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

void native_system_contract_plugin::install(database& db) {
#define SET_HANDLERS(name) \
   db.set_validate_handler("sys", "sys", #name, &name ## _validate); \
   db.set_precondition_validate_handler("sys", "sys", #name, &name ## _validate_preconditions); \
   db.set_apply_handler("sys", "sys", #name, &name ## _apply)

   SET_HANDLERS(Transfer);
   SET_HANDLERS(CreateAccount);
}
}
