#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/exceptions.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace eos { namespace chain {

void message_validate_context::require_authorization(const types::AccountName& account) {

   auto itr = boost::find_if(msg.authorization, [&account](const types::AccountPermission& ap) {
      return ap.account == account;
   });

   EOS_ASSERT(itr != msg.authorization.end(), tx_missing_auth,
              "Required authorization ${auth} not found", ("auth", account));

   used_authorizations[itr - msg.authorization.begin()] = true;
}

void message_validate_context::require_scope(const types::AccountName& account)const {
   auto itr = boost::find_if(trx.scope, [&account](const auto& scope) {
      return scope == account;
   });

   EOS_ASSERT( itr != trx.scope.end(), tx_missing_scope,
              "Required scope ${scope} not declared by transaction", ("scope",account) );
}

void message_validate_context::require_recipient(const types::AccountName& account)const {
   auto itr = boost::find_if(msg.recipients, [&account](const auto& recipient) {
      return recipient == account;
   });

   EOS_ASSERT( itr != msg.recipients.end(), tx_missing_recipient,
              "Required recipient ${recipient} not declared by message", ("recipient",account)("recipients",msg.recipients) );
}

bool message_validate_context::all_authorizations_used() const {
   return boost::algorithm::all_of_equal(used_authorizations, true);
}

} } // namespace eos::chain
