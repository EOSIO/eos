#pragma once

#include <eos/chain/message.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/types/types.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class message_validate_context {
public:
   explicit message_validate_context(const chainbase::database& d, 
                                     const chain::Transaction& t,
                                     const chain::Message& m, 
                                     types::AccountName c )
      :db(d),trx(t),msg(m),code(c),used_authorizations(msg.authorization.size(), false){}

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

   const chainbase::database&   db;  ///< database where state is stored
   const chain::Transaction&    trx; ///< used to gather the valid read/write scopes
   const chain::Message&        msg; ///< message being applied
   types::AccountName           code; ///< the code that is currently running

   ///< Parallel to msg.authorization; tracks which permissions have been used while processing the message
   vector<bool>                 used_authorizations;
};

class precondition_validate_context : public message_validate_context {
public:
   precondition_validate_context(const chainbase::database& db, 
                                 const chain::Transaction& t,
                                 const chain::Message& m, 
                                 const types::AccountName& code)
      :message_validate_context(db, t, m, code){}
};

class apply_context : public precondition_validate_context {
    public:
      apply_context(chainbase::database& db, 
                    const chain::Transaction& t,
                    const chain::Message& m, 
                    const types::AccountName& code)
         :precondition_validate_context(db,t,m,code),mutable_db(db){}

      types::String get(types::String key)const;
      void set(types::String key, types::String value);
      void remove(types::String key);

      std::deque<eos::chain::generated_transaction> applied; ///< sync calls made 
      std::deque<eos::chain::generated_transaction> generated; ///< async calls requested

      chainbase::database& mutable_db;
};

using message_validate_handler = std::function<void(message_validate_context&)>;
using precondition_validate_handler = std::function<void(precondition_validate_context&)>;
using apply_handler = std::function<void(apply_context&)>;

} } // namespace eos::chain
