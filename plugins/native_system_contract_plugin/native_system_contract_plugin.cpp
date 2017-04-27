#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace eos {
using namespace chain;

void Transfer_validate(chain::message_validate_context& context) {
   auto transfer = context.msg.as<Transfer>();
   EOS_ASSERT(context.msg.has_notify(transfer.to), message_validate_exception, "Must notify recipient of transfer");
}
void Transfer_validate_preconditions(chain::precondition_validate_context& context) {
   const auto& db = context.db;
   auto transfer = context.msg.as<Transfer>();
   const auto& from = db.get_account(context.msg.sender);
   EOS_ASSERT(from.balance >= transfer.amount, message_precondition_exception, "Insufficient Funds",
              ("from.balance",from.balance)("transfer.amount",transfer.amount));
}
void Transfer_apply(chain::apply_context& context) {
   auto& db = context.mutable_db;
   auto transfer = context.msg.as<Transfer>();
   const auto& from = db.get_account(context.msg.sender);
   const auto& to = db.get_account(transfer.to);
   db.modify(from, [&](account_object& a) {
      a.balance -= transfer.amount;
   });
   db.modify(to, [&](account_object& a) {
      a.balance += transfer.amount;
   });
}

void CreateAccount_validate(chain::message_validate_context& context) {
   auto create = context.msg.as<CreateAccount>();
   EOS_ASSERT(create.owner.validate(), message_validate_exception, "Invalid owner authority");
   EOS_ASSERT(create.active.validate(), message_validate_exception, "Invalid active authority");
}
void CreateAccount_validate_preconditions(chain::precondition_validate_context& context) {
   const auto& db = context.db;
   auto create = context.msg.as<CreateAccount>();

   auto existing_account = db.find<account_object, by_name>(create.new_account);
   EOS_ASSERT(existing_account == nullptr, message_precondition_exception,
              "Cannot create account named ${name}, as that name is already taken",
              ("name", create.new_account));

   const auto& creator = db.get_account(context.msg.sender);
   EOS_ASSERT(creator.balance >= create.initial_balance, message_precondition_exception, "Insufficient Funds");

   for (const auto& account : create.owner.referenced_accounts())
      db.get_account(account);
   for (const auto& account : create.active.referenced_accounts())
      db.get_account(account);
}
void CreateAccount_apply(chain::apply_context& context) {
   auto& db = context.mutable_db;
   auto create = context.msg.as<CreateAccount>();
   db.modify(db.get_account(context.msg.sender), [&create](account_object& a) {
      a.balance -= create.initial_balance;
   });
   const auto& new_account = db.create<account_object>([&create](account_object& a) {
      a.name = create.new_account;
      a.balance = create.initial_balance;
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
