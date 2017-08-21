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

} } // namespace eos::chain
