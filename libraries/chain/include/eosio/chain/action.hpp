#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

   struct permission_level {
      account_name    actor;
      permission_name permission;
   };

   inline bool operator== (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) == std::tie(rhs.actor, rhs.permission);
   }

   inline bool operator!= (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) != std::tie(rhs.actor, rhs.permission);
   }

   inline bool operator< (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) < std::tie(rhs.actor, rhs.permission);
   }

   inline bool operator<= (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) <= std::tie(rhs.actor, rhs.permission);
   }

   inline bool operator> (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) > std::tie(rhs.actor, rhs.permission);
   }

   inline bool operator>= (const permission_level& lhs, const permission_level& rhs) {
      return std::tie(lhs.actor, lhs.permission) >= std::tie(rhs.actor, rhs.permission);
   }

   /**
    *  An action is performed by an actor, aka an account. It may
    *  be created explicitly and authorized by signatures or might be
    *  generated implicitly by executing application code.
    *
    *  This follows the design pattern of React Flux where actions are
    *  named and then dispatched to one or more action handlers (aka stores).
    *  In the context of eosio, every action is dispatched to the handler defined
    *  by account 'scope' and function 'name', but the default handler may also
    *  forward the action to any number of additional handlers. Any application
    *  can write a handler for "scope::name" that will get executed if and only if
    *  this action is forwarded to that application.
    *
    *  Each action may require the permission of specific actors. Actors can define
    *  any number of permission levels. The actors and their respective permission
    *  levels are declared on the action and validated independently of the executing
    *  application code. An application code will check to see if the required authorization
    *  were properly declared when it executes.
    */
   struct action_base {
      account_name             account;
      action_name              name;
      vector<permission_level> authorization;

      action_base() = default;

      action_base( account_name acnt, action_name act, const vector<permission_level>& auth )
         : account(acnt), name(act), authorization(auth) {}
      action_base( account_name acnt, action_name act, vector<permission_level>&& auth )
         : account(acnt), name(act), authorization(std::move(auth)) {}
   };

   struct action : public action_base {
      bytes                      data;

      action() = default;

      template<typename T, std::enable_if_t<std::is_base_of<bytes, T>::value, int> = 1>
      action( vector<permission_level> auth, const T& value )
         : action_base( T::get_account(), T::get_name(), std::move(auth) ) {
         data.assign(value.data(), value.data() + value.size());
      }

      template<typename T, std::enable_if_t<!std::is_base_of<bytes, T>::value, int> = 1>
      action( vector<permission_level> auth, const T& value )
         : action_base( T::get_account(), T::get_name(), std::move(auth) ) {
         data = fc::raw::pack(value);
      }

      action( vector<permission_level> auth, account_name account, action_name name, const bytes& data )
            : action_base(account, name, std::move(auth)), data(data)  {
      }

      template<typename T>
      T data_as()const {
         EOS_ASSERT( account == T::get_account(), action_type_exception, "account is not consistent with action struct" );
         EOS_ASSERT( name == T::get_name(), action_type_exception, "action name is not consistent with action struct"  );
         return fc::raw::unpack<T>(data);
      }
   };

   template <typename Hasher>
   auto generate_action_digest(Hasher&& hash, const action& act, const vector<char>& action_output) {
      using hash_type = decltype(hash(nullptr, 0));
      hash_type hashes[2];
      const action_base* base = &act;
      const auto action_base_size   = fc::raw::pack_size(*base);
      const auto action_input_size  = fc::raw::pack_size(act.data);
      const auto action_output_size = fc::raw::pack_size(action_output);
      const auto rhs_size           = action_input_size + action_output_size;
      std::vector<char> buff;
      buff.reserve(std::max(action_base_size, rhs_size));
      {
         buff.resize(action_base_size);
         fc::datastream<char*> ds(buff.data(), action_base_size);
         fc::raw::pack(ds, *base);
         hashes[0] = hash(buff.data(), action_base_size);
      }
      {
         buff.resize(rhs_size);
         fc::datastream<char*> ds(buff.data(), rhs_size);
         fc::raw::pack(ds, act.data);
         fc::raw::pack(ds, action_output);
         hashes[1] = hash(buff.data(), rhs_size);
      }
      auto hashes_size = fc::raw::pack_size(hashes[0]) + fc::raw::pack_size(hashes[1]);
      buff.resize(hashes_size); // may cause reallocation but in practice will be unlikely
      fc::datastream<char*> ds(buff.data(), hashes_size);
      fc::raw::pack(ds, hashes[0]);
      fc::raw::pack(ds, hashes[1]);
      return hash(buff.data(), hashes_size);
   }

   struct action_notice : public action {
      account_name receiver;
   };

} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::permission_level, (actor)(permission) )
FC_REFLECT( eosio::chain::action_base, (account)(name)(authorization) )
FC_REFLECT_DERIVED( eosio::chain::action, (eosio::chain::action_base), (data) )
