/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {
   /**
    * @brief The permission_link_object class assigns permission_objects to message types
    *
    * This class records the links from contracts and message types within those contracts to permission_objects
    * defined by the user to record the authority required to execute those messages within those contracts. For
    * example, suppose we have a contract called "currency" and that contract defines a message called "transfer".
    * Furthermore, suppose a user, "joe", has a permission level called "money" and joe would like to require that
    * permission level in order for his account to invoke currency.transfer. To do this, joe would create a
    * permission_link_object for his account with "currency" as the code account, "transfer" as the message_type, and
    * "money" as the required_permission. After this, in order to validate, any message to "currency" of type
    * "transfer" that requires joe's approval would require signatures sufficient to satisfy joe's "money" authority.
    *
    * Accounts may set links to individual message types, or may set default permission requirements for all messages
    * to a given contract. To set the default for all messages to a given contract, set @ref message_type to the empty
    * string. When looking up which permission to use, if a link is found for a particular {account, code,
    * message_type} triplet, then the required_permission from that link is used. If no such link is found, but a link
    * is found for {account, code, ""}, then the required_permission from that link is used. If no such link is found,
    * account's active authority is used.
    */
   class permission_link_object : public chainbase::object<permission_link_object_type, permission_link_object> {
      OBJECT_CTOR(permission_link_object)

      id_type        id;
      /// The account which is defining its permission requirements
      account_name    account;
      /// The contract which account requires @ref required_permission to invoke
      account_name    code; /// TODO: rename to scope
      /// The message type which account requires @ref required_permission to invoke
      /// May be empty; if so, it sets a default @ref required_permission for all messages to @ref code
      action_name       message_type;
      /// The permission level which @ref account requires for the specified message types
      permission_name required_permission;
   };

   struct by_action_name;
   struct by_permission_name;
   using permission_link_index = chainbase::shared_multi_index_container<
      permission_link_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(permission_link_object, permission_link_object::id_type, id)
         >,
         ordered_unique<tag<by_action_name>,
            composite_key<permission_link_object,
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, account_name, account),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, account_name, code),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, action_name, message_type)
            >
         >,
         ordered_unique<tag<by_permission_name>,
            composite_key<permission_link_object,
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, account_name, account),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, permission_name, required_permission),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, account_name, code),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, action_name, message_type)
            >
         >
      >
   >;

   namespace config {
      template<>
      struct billable_size<permission_link_object> {
         static const uint64_t overhead = overhead_per_row_per_index_ram_bytes * 3; ///< 3x indices id, action, permission
         static const uint64_t value = 40 + overhead; ///< fixed field + overhead
      };
   }
} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_link_object, eosio::chain::permission_link_index)

FC_REFLECT(eosio::chain::permission_link_object, (account)(code)(message_type)(required_permission))
