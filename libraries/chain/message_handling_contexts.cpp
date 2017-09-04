#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/chain_controller.hpp>

#include <eos/utilities/parallel_markers.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void apply_context::require_authorization(const types::AccountName& account) {
   auto itr = boost::find_if(msg.authorization, [&account](const auto& auth) { return auth.account == account; });
   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Transaction is missing required authorization from ${acct}", ("acct", account));
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

pending_transaction& apply_context::get_pending_transaction(pending_transaction::handle_type handle) {
   auto itr = boost::find_if(pending_transactions, [&](const auto& trx) { return trx.handle == handle; });
   EOS_ASSERT(itr != pending_transactions.end(), tx_unknown_argument,
              "Transaction refers to non-existant/destroyed pending transaction");
   return *itr;
}

const int Max_pending_transactions = 4;
const uint32_t Max_pending_transaction_size = 16 * 1024;
const auto Pending_transaction_expiration = fc::seconds(21 * 3); 

pending_transaction& apply_context::create_pending_transaction() {
   EOS_ASSERT(pending_transactions.size() < Max_pending_transactions, tx_resource_exhausted,
              "Transaction is attempting to create too many pending transactions. The max is ${max}", ("max", Max_pending_transactions));)
   
   pending_transaction::handle handle = next_pending_transaction_serial++;
   pending_transactions.push_back({handle});
   return pending_transaction.back();
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

types::Transaction apply_context::pending_transaction::as_transaction() const {
   decltype(types::Transaction::refBlockNum) head_block_num = chain_controller.head_block_num();
   decltype(types::Transaction::refBlockRef) head_block_ref = fc::endian_reverse_u32(chain_controller.head_block_ref()._hash[0]);
   decltype(types::Transaction::expiration) expiration = chain_controller.head_block_time() + Pending_transaction_expiration;
   return types::Transaction(head_block_num, head_block_ref, expiration, scopes, read_scopes, messages);
}

void apply_context::pending_transaction::check_size() const {
   auto trx = as_transaction();
   EOS_ASSERT(fc::raw::pack_size(trx) <= Max_pending_transaction_size, tx_resource_exhausted,
              "Transaction is attempting to create a transaction which is too large. The max size is ${max} bytes", ("max", Max_pending_transaction_size));
}

} } // namespace eos::chain
