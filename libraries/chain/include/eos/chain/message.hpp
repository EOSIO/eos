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
struct message {
   /// The account which sent the message
   AccountName         sender;

   /// The account to receive the message
   AccountName         recipient;

   /// Other accounts to notify about this message
   vector<AccountName> notify;

   /**
    * Every contract defines the set of types that it accepts, these types are
    * scoped according to the recipient. This means two contracts can can define
    * two different types with the same name.
    */
   types::TypeName      type;

   /// The message contents
   vector<char>         data;

   template<typename T>
   void set( const TypeName& t, const T& value ) {
      type = t;
      data = fc::raw::pack( value );
   }
   template<typename T>
   T as()const {
      return fc::raw::unpack<T>(data);
   }
   bool has_notify( const AccountName& n )const {
      for( const auto& no : notify )
         if( no == n ) return true;
      return false; 
   }
};



} } // namespace eos::chain

FC_REFLECT(eos::chain::message, (sender)(recipient)(notify)(type)(data))
