#pragma once

#include <eos/chain/message.hpp>

#include <eos/types/types.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class message_validate_context {
public:
   explicit message_validate_context(const chain::Message& m)
      :msg(m){}

   const chain::Message& msg;
};

class precondition_validate_context : public message_validate_context {
public:
   precondition_validate_context(const chainbase::database& db, const chain::Message& m, const types::AccountName& r)
      :message_validate_context(m),recipient(r),db(db){}


   const types::AccountName& recipient;
   const chainbase::database& db;
};

class apply_context : public precondition_validate_context {
public:
   apply_context(chainbase::database& db, const chain::Message& m, const types::AccountName& recipient)
      :precondition_validate_context(db,m,recipient),mutable_db(db){}

   types::String get(types::String key)const;
   void set(types::String key, types::String value);
   void remove(types::String key);

   chainbase::database& mutable_db;
};

using message_validate_handler = std::function<void(message_validate_context&)>;
using precondition_validate_handler = std::function<void(precondition_validate_context&)>;
using apply_handler = std::function<void(apply_context&)>;

} } // namespace eos::chain
