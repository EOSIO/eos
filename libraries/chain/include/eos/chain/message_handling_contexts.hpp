#pragma once

#include <eos/chain/message.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/types/types.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class message_validate_context {
public:
   explicit message_validate_context(const chainbase::database& d, const chain::Message& m, types::AccountName s)
      :msg(m),db(d),scope(s),used_authorizations(msg.authorization.size(), false){}

   /**
    * @brief Require @ref account to have approved of this message
    * @param account The account whose approval is required
    *
    * This method will check that @ref account is listed in the message's declared authorizations, and marks the
    * authorization as used. Note that all authorizations on a message must be used, or the message is invalid.
    *
    * @throws tx_missing_auth If no sufficient permission was found
    */
   void require_authorization(const types::AccountName& account);
   bool all_authorizations_used() const;

   const chain::Message&        msg;
   const chainbase::database&   db;
   types::AccountName           scope;
   ///< Parallel to msg.authorization; tracks which permissions have been used while processing the message
   vector<bool>                 used_authorizations;
};

class precondition_validate_context : public message_validate_context {
public:
   precondition_validate_context(const chainbase::database& db, const chain::Message& m, const types::AccountName& scope)
      :message_validate_context(db, m, scope){}
};

class apply_context : public precondition_validate_context {
public:
   apply_context(chainbase::database& db, const chain::Message& m, const types::AccountName& scope)
      :precondition_validate_context(db,m,scope),mutable_db(db){}

   types::String get(types::String key)const;
   void set(types::String key, types::String value);
   void remove(types::String key);

   std::deque<eos::chain::generated_transaction> generated;

   chainbase::database& mutable_db;
};

using message_validate_handler = std::function<void(message_validate_context&)>;
using precondition_validate_handler = std::function<void(precondition_validate_context&)>;
using apply_handler = std::function<void(apply_context&)>;

} } // namespace eos::chain
