#pragma once

#include <eos/chain/message.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/types/types.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class chain_controller;
class message_validate_context {
public:
   explicit message_validate_context(const chain_controller& control,
                                     const chainbase::database& d, 
                                     const chain::Transaction& t,
                                     const chain::Message& m, 
                                     types::AccountName c )
      :controller(control),db(d),trx(t),msg(m),code(c),used_authorizations(msg.authorization.size(), false){}

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
   void require_scope(const types::AccountName& account)const;
   void require_recipient(const types::AccountName& account)const;
   bool all_authorizations_used() const;

   const chain_controller&      controller;
   const chainbase::database&   db;  ///< database where state is stored
   const chain::Transaction&    trx; ///< used to gather the valid read/write scopes
   const chain::Message&        msg; ///< message being applied
   types::AccountName           code; ///< the code that is currently running


   int32_t load_i64( Name scope, Name code, Name table, Name Key, char* data, uint32_t maxlen );


   int32_t front_primary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t back_primary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t front_secondary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t back_secondary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t load_primary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t lowerbound_primary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );
   int32_t lowerbound_secondary_i128i128( Name scope, Name code, Name table, 
                                    uint128_t* primary, uint128_t* secondary, char* data, uint32_t maxlen );

   ///< Parallel to msg.authorization; tracks which permissions have been used while processing the message
   vector<bool>                 used_authorizations;
};

class precondition_validate_context : public message_validate_context {
public:
   precondition_validate_context(const chain_controller& con,
                                 const chainbase::database& db, 
                                 const chain::Transaction& t,
                                 const chain::Message& m, 
                                 const types::AccountName& code)
      :message_validate_context(con, db, t, m, code){}
};

class apply_context : public precondition_validate_context {
    public:
      apply_context(chain_controller& con,
                    chainbase::database& db, 
                    const chain::Transaction& t,
                    const chain::Message& m, 
                    const types::AccountName& code)
         :precondition_validate_context(con,db,t,m,code),mutable_controller(con),mutable_db(db){}

      int32_t store_i64( Name scope, Name table, Name key, const char* data, uint32_t len);
      int32_t remove_i64( Name scope, Name table, Name key );
      int32_t remove_i128i128( Name scope, Name table, uint128_t primary, uint128_t secondary );
      int32_t store_i128i128( Name scope, Name table, uint128_t primary, uint128_t secondary,
                              const char* data, uint32_t len );

      std::deque<eos::chain::generated_transaction> applied; ///< sync calls made 
      std::deque<eos::chain::generated_transaction> generated; ///< async calls requested

      chain_controller&    mutable_controller;
      chainbase::database& mutable_db;
};

using message_validate_handler = std::function<void(message_validate_context&)>;
using precondition_validate_handler = std::function<void(precondition_validate_context&)>;
using apply_handler = std::function<void(apply_context&)>;

} } // namespace eos::chain
