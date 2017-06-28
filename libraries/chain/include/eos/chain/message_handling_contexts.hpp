#pragma once

#include <eos/chain/message.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/types/types.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class message_validate_context {
public:
   explicit message_validate_context(const chainbase::database& d, const chain::Message& m, types::AccountName s)
      :msg(m),db(d),scope(s){}

   const chain::Message&        msg;
   const chainbase::database&   db;    /// required only for loading the contract code
   types::AccountName           scope; /// the contract that is being called
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
