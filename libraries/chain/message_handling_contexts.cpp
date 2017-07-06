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

bool message_validate_context::all_authorizations_used() const {
   return boost::algorithm::all_of_equal(used_authorizations, true);
}

} } // namespace eos::chain
