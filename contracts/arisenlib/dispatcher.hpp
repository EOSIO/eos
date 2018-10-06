#pragma once
#include <arisenlib/print.hpp>
#include <arisenlib/action.hpp>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/mp11/tuple.hpp>
#define N(X) ::arisen::string_to_name(#X)
namespace arisen {

   template<typename Contract, typename FirstAction>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract().on( unpack_action_data<FirstAction>() );
         return true;
      }
      return false;
   }


   /**
    * This method will dynamically dispatch an incoming set of actions to
    *
    * ```
    * static Contract::on( ActionType )
    * ```
    *
    * For this to work the Actions must be derived from arisen::contract
    *
    */
   template<typename Contract, typename FirstAction, typename SecondAction, typename... Actions>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract().on( unpack_action_data<FirstAction>() );
         return true;
      }
      return arisen::dispatch<Contract,SecondAction,Actions...>( code, act );
   }

   /**
    * @defgroup dispatcher Dispatcher API
    * @brief Defines functions to dispatch action to proper action handler inside a contract
    * @ingroup contractdev
    */

   /**
    * @defgroup dispatchercpp Dispatcher C++ API
    * @brief Defines C++ functions to dispatch action to proper action handler inside a contract
    * @ingroup dispatcher
    * @{
    */

   /**
    * Unpack the received action and execute the correponding action handler
    *
    * @brief Unpack the received action and execute the correponding action handler
    * @tparam T - The contract class that has the correponding action handler, this contract should be derived from arisen::contract
    * @tparam Q - The namespace of the action handler function
    * @tparam Args - The arguments that the action handler accepts, i.e. members of the action
    * @param obj - The contract object that has the correponding action handler
    * @param func - The action handler
    * @return true
    */
   template<typename T, typename Q, typename... Args>
   bool execute_action( T* obj, void (Q::*func)(Args...)  ) {
      size_t size = action_data_size();

      //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
      constexpr size_t max_stack_buffer_size = 512;
      void* buffer = nullptr;
      if( size > 0 ) {
         buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
         read_action_data( buffer, size );
      }

      auto args = unpack<std::tuple<std::decay_t<Args>...>>( (char*)buffer, size );

      if ( max_stack_buffer_size < size ) {
         free(buffer);
      }

      auto f2 = [&]( auto... a ){
         (obj->*func)( a... );
      };

      boost::mp11::tuple_apply( f2, args );
      return true;
   }
 /// @}  dispatcher

// Helper macro for ARISEN_API
#define ARISEN_API_CALL( r, OP, elem ) \
   case ::arisen::string_to_name( BOOST_PP_STRINGIZE(elem) ): \
      arisen::execute_action( &thiscontract, &OP::elem ); \
      break;

// Helper macro for ARISEN_ABI
#define ARISEN_API( TYPE,  MEMBERS ) \
   BOOST_PP_SEQ_FOR_EACH( ARISEN_API_CALL, TYPE, MEMBERS )

/**
 * @addtogroup dispatcher
 * @{
 */

/**
 * Convenient macro to create contract apply handler
 * To be able to use this macro, the contract needs to be derived from arisen::contract
 *
 * @brief Convenient macro to create contract apply handler
 * @param TYPE - The class name of the contract
 * @param MEMBERS - The sequence of available actions supported by this contract
 *
 * Example:
 * @code
 * ARISEN_ABI( arisen::bios, (setpriv)(setalimits)(setglimits)(setprods)(reqauth) )
 * @endcode
 */
#define ARISEN_ABI( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( action == N(onerror)) { \
         /* onerror is only valid if it is for the "arisen" code account and authorized by "arisen"'s "active permission */ \
         arisen_assert(code == N(arisen), "onerror action's are only valid from the \"arisen\" system account"); \
      } \
      if( code == self || action == N(onerror) ) { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            ARISEN_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: arisen_exit(0); */ \
      } \
   } \
} \
 /// @}  dispatcher


   /*
   template<typename T>
   struct dispatcher {
      dispatcher( account_name code ):_contract(code){}

      template<typename FuncPtr>
      void dispatch( account_name action, FuncPtr ) {
      }

      T contract;
   };

   void dispatch( account_name code, account_name action,
   */

}
