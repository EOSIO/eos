/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/action.h>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>

#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

namespace eosio {

   /**
    * @defgroup actioncppapi Action C++ API
    * @ingroup actionapi
    * @brief Type-safe C++ wrapers for Action C API
    *
    * @note There are some methods from the @ref actioncapi that can be used directly from C++
    *
    * @{
    */

   /**
    *
    *  This method unpacks the current action at type T.
    *
    *  @brief Interpret the action body as type T
    *
    *  Example:
    *  @code
    *  struct dummy_action {
    *    char a; //1
    *    unsigned long long b; //8
    *    int  c; //4
    *
    *    EOSLIB_SERIALIZE( dummy_action, (a)(b)(c) )
    *  };
    *  dummy_action msg = unpack_action_data<dummy_action>();
    *  @endcode
    */
   template<typename T>
   T unpack_action_data() {
      char buffer[action_data_size()];
      read_action_data( buffer, sizeof(buffer) );
      return unpack<T>( buffer, sizeof(buffer) );
   }

   using ::require_auth;
   using ::require_recipient;

   /**
    *  All of the listed accounts will be added to the set of accounts to be notified
    *
    *  This helper method enables you to add multiple accounts to accounts to be notified list with a single
    *  call rather than having to call the similar C API multiple times.
    *
    *  @note action.code is also considered as part of the set of notified accounts
    *
    *  @brief Verify specified accounts exist in the set of notified accounts
    *
    *  Example:
    *  @code
    *  require_recipient(N(Account1), N(Account2), N(Account3)); // throws exception if any of them not in set.
    *  @endcode
    */
   template<typename... accounts>
   void require_recipient( account_name name, accounts... remaining_accounts ){
      require_recipient( name );
      require_recipient( remaining_accounts... );
   }

   struct permission_level {
      permission_level( account_name a, permission_name p ):actor(a),permission(p){}
      permission_level(){}

      account_name    actor;
      permission_name permission;

      friend bool operator == ( const permission_level& a, const permission_level& b ) {
         return std::tie( a.actor, a.permission ) == std::tie( b.actor, b.permission );
      }

      EOSLIB_SERIALIZE( permission_level, (actor)(permission) )
   };

   void require_auth(const permission_level& level) {
      require_auth2( level.actor, level.permission );
   }

   /**
    * This is the packed representation of an action along with
    * meta-data about the authorization levels.
    */
   struct action {
      account_name               account;
      action_name                name;
      vector<permission_level>   authorization;
      bytes                      data;

      action() = default;

      /**
       *  @tparam Action - a type derived from action_meta<Scope,Name>
       *  @param value - will be serialized via pack into data
       */
      template<typename Action>
      action( vector<permission_level>&& auth, const Action& value ) {
         account       = Action::get_account();
         name          = Action::get_name();
         authorization = move(auth);
         data          = pack(value);
      }

      /**
       *  @tparam Action - a type derived from action_meta<Scope,Name>
       *  @param value - will be serialized via pack into data
       */
      template<typename Action>
      action( const permission_level& auth, const Action& value )
      :authorization(1,auth) {
         account       = Action::get_account();
         name          = Action::get_name();
         data          = pack(value);
      }

      /**
       *  @tparam Action - a type derived from action_meta<Scope,Name>
       *  @param value - will be serialized via pack into data
       */
      template<typename Action>
      action( const Action& value ) {
         account       = Action::get_account();
         name          = Action::get_name();
         data          = pack(value);
      }

      /**
       *  @tparam T - the type of the action data
       *  @param auth - a single permission_level to be used as the authorization of the action
       *  @param a - name of the contract account
       *  @param n - name of the action
       *  @param value - will be serialized via pack into data
       */
      template<typename T>
      action( const permission_level& auth, account_name a, action_name n, T&& value )
      :account(a), name(n), authorization(1,auth), data(pack(std::forward<T>(value))) {}

      /**
       *  @tparam T - the type of the action data
       *  @param auths - vector permission_levels defining the authorizations of the action
       *  @param a - name of the contract account
       *  @param n - name of the action
       *  @param value - will be serialized via pack into data
       */
      template<typename T>
      action( vector<permission_level> auths, account_name a, action_name n, T&& value )
      :account(a), name(n), authorization(std::move(auths)), data(pack(std::forward<T>(value))) {}

      EOSLIB_SERIALIZE( action, (account)(name)(authorization)(data) )

      void send() const {
         auto serialize = pack(*this);
         ::send_inline(serialize.data(), serialize.size());
      }

      void send_context_free() const {
         eosio_assert( authorization.size() == 0, "context free actions cannot have authorizations");
         auto serialize = pack(*this);
         ::send_context_free_inline(serialize.data(), serialize.size());
      }

      /**
       * Retrieve the unpacked data as T
       * @tparam T expected type of data
       * @return the action data
       */
      template<typename T>
      T data_as() {
         eosio_assert( name == T::get_name(), "Invalid name" );
         eosio_assert( account == T::get_account(), "Invalid account" );
         return unpack<T>( &data[0], data.size() );
      }

   };

   template<uint64_t Account, uint64_t Name>
   struct action_meta {
      static uint64_t get_account() { return Account; }
      static uint64_t get_name()  { return Name; }
   };

   template<typename... Args>
   void dispatch_inline( account_name code, action_name act,
                         vector<permission_level> perms,
                         std::tuple<Args...> args ) {
      action( perms, code, act, std::move(args) ).send();
   }

   template<typename, uint64_t>
   struct inline_dispatcher;

   template<typename T, uint64_t Name, typename... Args>
   struct inline_dispatcher<void(T::*)(Args...), Name> {
      static void call(account_name code, const permission_level& perm, std::tuple<Args...> args) {
         dispatch_inline(code, Name, vector<permission_level>(1, perm), std::move(args));
      }
      static void call(account_name code, vector<permission_level> perms, std::tuple<Args...> args) {
         dispatch_inline(code, Name, std::move(perms), std::move(args));
      }
   };

 ///@} actioncpp api

} // namespace eosio

#define INLINE_ACTION_SENDER3( CONTRACT_CLASS, FUNCTION_NAME, ACTION_NAME  )\
::eosio::inline_dispatcher<decltype(&CONTRACT_CLASS::FUNCTION_NAME), ACTION_NAME>::call

#define INLINE_ACTION_SENDER2( CONTRACT_CLASS, NAME )\
INLINE_ACTION_SENDER3( CONTRACT_CLASS, NAME, ::eosio::string_to_name(#NAME) )

#define INLINE_ACTION_SENDER(...) BOOST_PP_OVERLOAD(INLINE_ACTION_SENDER,__VA_ARGS__)(__VA_ARGS__)

#define SEND_INLINE_ACTION( CONTRACT, NAME, ... )\
INLINE_ACTION_SENDER(std::decay_t<decltype(CONTRACT)>, NAME)( (CONTRACT).get_self(),\
BOOST_PP_TUPLE_ENUM(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) );


#define ACTION( CODE, NAME ) struct NAME : ::eosio::action_meta<CODE, ::eosio::string_to_name(#NAME) >
