#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace eosioboot {

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   /**
    * A weighted permission.
    *
    * @details Defines a weighted permission, that is a permission which has a weight associated.
    * A permission is defined by an account name plus a permission name. The weight is going to be
    * used against a threshold, if the weight is equal or greater than the threshold set then authorization
    * will pass.
    */
   struct permission_level_weight {
      permission_level  permission;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   /**
    * Weighted key.
    *
    * @details A weighted key is defined by a public key and an associated weight.
    */
   struct key_weight {
      eosio::public_key  key;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   /**
    * Wait weight.
    *
    * @details A wait weight is defined by a number of seconds to wait for and a weight.
    */
   struct wait_weight {
      uint32_t           wait_sec;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
   };

   /**
    * Blockchain authority.
    *
    * @details An authority is defined by:
    * - a vector of key_weights (a key_weight is a public key plus a weight),
    * - a vector of permission_level_weights, (a permission_level is an account name plus a permission name)
    * - a vector of wait_weights (a wait_weight is defined by a number of seconds to wait and a weight)
    * - a threshold value
    */
   struct authority {
      uint32_t                              threshold = 0;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;
      std::vector<wait_weight>              waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
   };

   /**
    * @defgroup eosioboot eosio.boot
    * @ingroup eosiocontracts
    *
    * eosio.boot is a extremely minimalistic system contract that only supports the native actions and an
    * activate action that allows activating desired protocol features prior to deploying a system contract
    * with more features such as eosio.bios or eosio.system.
    *
    * @{
    */
   class [[eosio::contract("eosio.boot")]] boot : public eosio::contract {
      public:
         using contract::contract;
         /**
          * @{
          * These actions map one-on-one with the ones defined in
          * [Native Action Handlers](@ref native_action_handlers) section.
          * They are present here so they can show up in the abi file and thus user can send them
          * to this contract, but they have no specific implementation at this contract level,
          * they will execute the implementation at the core level and nothing else.
          */
         /**
          * New account action
          *
          * @details Creates a new account.
          *
          * @param creator - the creator of the account
          * @param name - the name of the new account
          * @param owner - the authority for the owner permission of the new account
          * @param active - the authority for the active permission of the new account
          */
         [[eosio::action]]
         void newaccount( name             creator,
                          name             name,
                          ignore<authority> owner,
                          ignore<authority> active) {}
         /**
          * Update authorization action.
          *
          * @details Updates pemission for an account.
          *
          * @param account - the account for which the permission is updated,
          * @param pemission - the permission name which is updated,
          * @param parem - the parent of the permission which is updated,
          * @param aut - the json describing the permission authorization.
          */
         [[eosio::action]]
         void updateauth(  ignore<name>  account,
                           ignore<name>  permission,
                           ignore<name>  parent,
                           ignore<authority> auth ) {}

         /**
          * Delete authorization action.
          *
          * @details Deletes the authorization for an account's permision.
          *
          * @param account - the account for which the permission authorization is deleted,
          * @param permission - the permission name been deleted.
          */
         [[eosio::action]]
         void deleteauth( ignore<name>  account,
                          ignore<name>  permission ) {}

         /**
          * Link authorization action.
          *
          * @details Assigns a specific action from a contract to a permission you have created. Five system
          * actions can not be linked `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, and `canceldelay`.
          * This is useful because when doing authorization checks, the EOSIO based blockchain starts with the
          * action needed to be authorized (and the contract belonging to), and looks up which permission
          * is needed to pass authorization validation. If a link is set, that permission is used for authoraization
          * validation otherwise then active is the default, with the exception of `eosio.any`.
          * `eosio.any` is an implicit permission which exists on every account; you can link actions to `eosio.any`
          * and that will make it so linked actions are accessible to any permissions defined for the account.
          *
          * @param account - the permission's owner to be linked and the payer of the RAM needed to store this link,
          * @param code - the owner of the action to be linked,
          * @param type - the action to be linked,
          * @param requirement - the permission to be linked.
          */
         [[eosio::action]]
         void linkauth(  ignore<name>    account,
                         ignore<name>    code,
                         ignore<name>    type,
                         ignore<name>    requirement  ) {}

         /**
          * Unlink authorization action.
          *
          * @details It's doing the reverse of linkauth action, by unlinking the given action.
          *
          * @param account - the owner of the permission to be unlinked and the receiver of the freed RAM,
          * @param code - the owner of the action to be unlinked,
          * @param type - the action to be unlinked.
          */
         [[eosio::action]]
         void unlinkauth( ignore<name>  account,
                          ignore<name>  code,
                          ignore<name>  type ) {}

         /**
          * Cancel delay action.
          *
          * @details Cancels a deferred transaction.
          *
          * @param canceling_auth - the permission that authorizes this action,
          * @param trx_id - the deferred transaction id to be cancelled.
          */
         [[eosio::action]]
         void canceldelay( ignore<permission_level> canceling_auth, ignore<checksum256> trx_id ) {}

         /**
          * Set code action.
          *
          * @details Sets the contract code for an account.
          *
          * @param account - the account for which to set the contract code.
          * @param vmtype - reserved, set it to zero.
          * @param vmversion - reserved, set it to zero.
          * @param code - the code content to be set, in the form of a blob binary..
          */
         [[eosio::action]]
         void setcode( name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char>& code ) {}

         /**
          * Set abi for contract.
          *
          * @details Set the abi for contract identified by `account` name.
          *
          * @param account - the name of the account to set the abi for
          * @param abi     - the abi hash represented as a vector of characters
          */
         [[eosio::action]]
         void setabi( name account, const std::vector<char>& abi ) {}

         /** @}*/

         /**
          * On error action.
          *
          * @details Notification of this action is delivered to the sender of a deferred transaction
          * when an objective error occurs while executing the deferred transaction.
          * This action is not meant to be called directly.
          *
          * @param sender_id - the id for the deferred transaction chosen by the sender,
          * @param sent_trx - the deferred transaction that failed.
          */
         [[eosio::action]]
         void onerror( ignore<uint128_t> sender_id, ignore<std::vector<char>> sent_trx );

         /**
          * Activates a protocol feature.
          *
          * @details Activates a protocol feature
          *
          * @param feature_digest - hash of the protocol feature to activate.
          */
         [[eosio::action]]
         void activate( const eosio::checksum256& feature_digest );

         /**
          * Asserts that a protocol feature has been activated.
          *
          * @details Asserts that a protocol feature has been activated
          *
          * @param feature_digest - hash of the protocol feature to check for activation.
          */
         [[eosio::action]]
         void reqactivated( const eosio::checksum256& feature_digest );

         using newaccount_action = action_wrapper<"newaccount"_n, &boot::newaccount>;
         using updateauth_action = action_wrapper<"updateauth"_n, &boot::updateauth>;
         using deleteauth_action = action_wrapper<"deleteauth"_n, &boot::deleteauth>;
         using linkauth_action = action_wrapper<"linkauth"_n, &boot::linkauth>;
         using unlinkauth_action = action_wrapper<"unlinkauth"_n, &boot::unlinkauth>;
         using canceldelay_action = action_wrapper<"canceldelay"_n, &boot::canceldelay>;
         using setcode_action = action_wrapper<"setcode"_n, &boot::setcode>;
         using setabi_action = action_wrapper<"setabi"_n, &boot::setabi>;
         using activate_action = action_wrapper<"activate"_n, &boot::activate>;
         using reqactivated_action = action_wrapper<"reqactivated"_n, &boot::reqactivated>;
   };
   /** @}*/ // end of @defgroup eosioboot eosio.boot
} /// namespace eosioboot
