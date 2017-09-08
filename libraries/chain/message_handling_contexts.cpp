#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/chain_controller.hpp>

#include <eos/utilities/parallel_markers.hpp>

#include <fc/bitutil.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void apply_context::require_authorization(const types::AccountName& account) {
   auto itr = boost::find_if(msg.authorization, [&account](const auto& auth) { return auth.account == account; });
   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Transaction is missing required authorization from ${acct}", ("acct", account));
   used_authorizations[itr - msg.authorization.begin()] = true;
}

void apply_context::require_authorization(const types::AccountName& account,const types::PermissionName& permission) {
   auto itr = boost::find_if(msg.authorization, [&account, &permission](const auto& auth) { return auth.account == account && auth.permission == permission; });
   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Transaction is missing required authorization from ${acct} with permission ${permission}", ("acct", account)("permission", permission));
   used_authorizations[itr - msg.authorization.begin()] = true;
}

void apply_context::require_scope(const types::AccountName& account)const {
   auto itr = boost::find_if(trx.scope, [&account](const auto& scope) {
      return scope == account;
   });

   if( controller.should_check_scope() ) {
      EOS_ASSERT( itr != trx.scope.end(), tx_missing_scope,
                 "Required scope ${scope} not declared by transaction", ("scope",account) );
   }
}

void apply_context::require_recipient(const types::AccountName& account) {
   if (account == msg.code)
      return;

   auto itr = boost::find_if(notified, [&account](const auto& recipient) {
      return recipient == account;
   });

   if (itr == notified.end()) {
      notified.push_back(account);
   }
}

bool apply_context::all_authorizations_used() const {
   return boost::algorithm::all_of_equal(used_authorizations, true);
}

vector<types::AccountPermission> apply_context::unused_authorizations() const {
   auto range = utilities::FilterDataByMarker(msg.authorization, used_authorizations, false);
   return {range.begin(), range.end()};
}

apply_context::pending_transaction& apply_context::get_pending_transaction(pending_transaction::handle_type handle) {
   auto itr = boost::find_if(pending_transactions, [&](const auto& trx) { return trx.handle == handle; });
   EOS_ASSERT(itr != pending_transactions.end(), tx_unknown_argument,
              "Transaction refers to non-existant/destroyed pending transaction");
   return *itr;
}

const auto Pending_transaction_expiration = fc::seconds(21 * 3);
const int Max_pending_messages = 4;
const int Max_pending_transactions = 4;

apply_context::pending_transaction& apply_context::create_pending_transaction() {
   EOS_ASSERT(pending_transactions.size() < Max_pending_transactions, tx_resource_exhausted,
              "Transaction is attempting to create too many pending transactions. The max is ${max}", ("max", Max_pending_transactions));
   
   
   pending_transaction::handle_type handle = next_pending_transaction_serial++;
   auto head_block_id = controller.head_block_id();
   decltype(pending_transaction::refBlockNum) head_block_num = fc::endian_reverse_u32(head_block_id._hash[0]);
   decltype(pending_transaction::refBlockPrefix) head_block_ref = head_block_id._hash[1];
   decltype(pending_transaction::expiration) expiration = controller.head_block_time() + Pending_transaction_expiration;
   pending_transactions.emplace_back(handle, *this, head_block_num, head_block_ref, expiration);
   return pending_transactions.back();
}

void apply_context::release_pending_transaction(pending_transaction::handle_type handle) {
   auto itr = boost::find_if(pending_transactions, [&](const auto& trx) { return trx.handle == handle; });
   EOS_ASSERT(itr != pending_transactions.end(), tx_unknown_argument,
              "Transaction refers to non-existant/destroyed pending transaction");

   auto last = pending_transactions.end() - 1;
   if (itr != last) {
      std::swap(itr, last);
   }
   pending_transactions.pop_back();
}

void apply_context::pending_transaction::check_size() const {
   const types::Transaction& trx = *this;
   const BlockchainConfiguration& chain_configuration = context.controller.get_global_properties().configuration;
   EOS_ASSERT(fc::raw::pack_size(trx) <= chain_configuration.maxGenTrxSize, tx_resource_exhausted,
              "Transaction is attempting to create a transaction which is too large. The max size is ${max} bytes", ("max", chain_configuration.maxGenTrxSize));
}

apply_context::pending_message& apply_context::get_pending_message(pending_message::handle_type handle) {
   auto itr = boost::find_if(pending_messages, [&](const auto& msg) { return msg.handle == handle; });
   EOS_ASSERT(itr != pending_messages.end(), tx_unknown_argument,
              "Transaction refers to non-existant/destroyed pending message");
   return *itr;
}

apply_context::pending_message& apply_context::create_pending_message(const AccountName& code, const FuncName& type, const Bytes& data) {
   const BlockchainConfiguration& chain_configuration = controller.get_global_properties().configuration;
   
   EOS_ASSERT(pending_messages.size() < Max_pending_messages, tx_resource_exhausted,
              "Transaction is attempting to create too many pending messages. The max is ${max}", ("max", Max_pending_messages));
   
   EOS_ASSERT(data.size() < chain_configuration.maxInlineMsgSize, tx_resource_exhausted,
              "Transaction is attempting to create a pending message that is too large. The max is ${max}", ("max", chain_configuration.maxInlineMsgSize));
   
   pending_message::handle_type handle = next_pending_message_serial++;
   pending_messages.emplace_back(handle, code, type, data);
   return pending_messages.back();
}

void apply_context::release_pending_message(pending_message::handle_type handle) {
   auto itr = boost::find_if(pending_messages, [&](const auto& trx) { return trx.handle == handle; });
   EOS_ASSERT(itr != pending_messages.end(), tx_unknown_argument,
              "Transaction refers to non-existant/destroyed pending message");

   auto last = pending_messages.end() - 1;
   if (itr != last) {
      std::swap(itr, last);
   }
   pending_messages.pop_back();
}

} } // namespace eos::chain
