/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>

namespace eosio { namespace chain {

/**
 * @brief The message struct defines a blockchain message
 *
#warning Outdated documentation should be fixed
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
struct message : public types::message {
   message() = default;
   template<typename T>
   message(const account_name& code, const vector<types::account_permission>& authorization, const types::func_name& type, T&& value)
      :types::message(code, type, authorization, bytes()) {
      set<T>(type, std::forward<T>(value));
   }

   message(const account_name& code, const vector<types::account_permission>& authorization, const types::func_name& type)
      :types::message(code, type, authorization, bytes()) {}

   message(const types::message& m) : types::message(m) {}

   template<typename T>
   void set_packed(const types::func_name& t, const T& value) {
      type = t;
      data.resize(sizeof(value));
      memcpy( data.data(), &value, sizeof(value) );
   }

   template<typename T>
   void set(const types::func_name& t, const T& value) {
      type = t;
      data = fc::raw::pack(value);
   }
   template<typename T>
   T as()const {
      return fc::raw::unpack<T>(data);
   }
};



} } // namespace eosio::chain

FC_REFLECT_DERIVED(eosio::chain::message, (eosio::types::message), )
