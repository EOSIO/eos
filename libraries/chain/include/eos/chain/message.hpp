#pragma once

#include <eos/chain/types.hpp>

namespace eos { namespace chain {

/**
 * @brief The message struct defines a blockchain message
 *
 * Messages are the heart of all activity on the blockchain,
 * -- all events and actions which take place in the chain are
 * recorded as messages. Messages are sent from one account 
 * (@ref sender) to another account (@ref recipient), and are
 * optionally also delivered to several other accounts (@ref notify).
 *
 * A message has a header that defines who sent it and who will 
 * be processing it. The message content is a binary blob,
 * @ref data, whose type is determined by @ref type, which is 
 * dynamic and defined by the scripting language.
 */
struct Message : public types::Message {
   Message() = default;
   template<typename T>
   Message(const AccountName& code, const vector<types::AccountName>& recipients,
           const vector<types::AccountPermission>& authorization, const TypeName& type, T&& value)
      : types::Message(code, recipients, authorization, {}, {}) {
      set<T>(type, std::forward<T>(value));
   }
   Message(const types::Message& m) : types::Message(m) {}

   template<typename T>
   void set(const TypeName& t, const T& value) {
      type = t;
      data = fc::raw::pack(value);
   }
   template<typename T>
   T as()const {
      return fc::raw::unpack<T>(data);
   }
   bool has_notify(const AccountName& n)const {
      for(const auto& no : recipients)
         if(no == n) return true;
      return false; 
   }

   template<typename Lambda>
   void for_each_handler(Lambda&& l)const {
      l(code);
      for(const auto& recipient : recipients)
         l(recipient);
   }

   types::AccountName recipient(UInt8 index) const {
      FC_ASSERT(index < recipients.size(), "Invalid recipient index: ${index}/${size}",
                ("index", index)("size", recipients.size()));
      return recipients.at(int(index));
   }
};



} } // namespace eos::chain

FC_REFLECT_DERIVED(eos::chain::Message, (eos::types::Message), )
