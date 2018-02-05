#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>

#include "multi_index_includes.hpp"


namespace eosio { namespace chain {

   /**
    *  This class accumulates an average value taking periodic inputs.
    *
    *  The average value returns to 0 after WindowMS without any updates and
    *  decays linerally if updates occur in the middle of a window before adding
    *  the new value.
    *
    *  The value stored is Precision times the sum of the inputs.
    */
   template<uint32_t WindowMs, uint64_t Precision = config::rate_limiting_precision>
   struct average_accumulator
   {
      time_point last_update;
      uint64_t   value = 0;

      /**
       * return the average value in rate_limiting_precision
       */
      uint64_t average()const { return value / WindowMs; }

      void add_usage( uint64_t units, time_point now )
      {
         if( now != last_update ) {
            auto elapsed = now - last_update;
            if( elapsed > fc::milliseconds(WindowMs) ) {
               value  = 0;
            } else if( elapsed > fc::days(0) ) {
               value *= (fc::milliseconds(WindowMs) - elapsed).count();
               value /=  fc::milliseconds(WindowMs).count();
            }
            last_update    = now;
         }
         value += units * Precision;
      }
   };


   /**
    * Every account that authorizes a transaction is billed for the full size of that transaction. This object
    * tracks the average usage of that account.
    */
   struct bandwidth_usage_object : public chainbase::object<bandwidth_usage_object_type, bandwidth_usage_object> {

      OBJECT_CTOR(bandwidth_usage_object)

      id_type          id;
      account_name     owner; 

      uint64_t net_weight  = 0;
      uint64_t cpu_weight  = 0; 
      uint64_t db_reserved_capacity = 0; /// bytes

      average_accumulator<config::bandwidth_average_window_ms> bytes;
      average_accumulator<config::compute_average_window_ms>   acts; ///< tracks a logical number of actions processed

      /**
       *  If 'owner' is a smart contract, then all increases / decreases in storage by that executing code should
       *  update this value.
       */
      uint64_t                                                 db_usage = 0;
   };

   /**
    *  Every account that authorizes a transaction calls add_usage( num_scopes_referenced^2, now ), for each
    *  of the scopes.  Referencing more scopes will result in exponential usage of compute bandwidth.
    */
   class compute_usage_object : public chainbase::object<compute_usage_object_type, compute_usage_object> {
      OBJECT_CTOR(compute_usage_object)

      id_type          id;
      account_name     owner;
      scope_name       scope;

      average_accumulator<config::compute_average_window_ms> actions;

      void add_usage( uint32_t actions, time_point now );
   };


   struct by_owner;
   using bandwidth_usage_index = chainbase::shared_multi_index_container<
      bandwidth_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<bandwidth_usage_object, bandwidth_usage_object::id_type, &bandwidth_usage_object::id>>,
         ordered_unique<tag<by_owner>, member<bandwidth_usage_object, account_name, &bandwidth_usage_object::owner> >
      >
   >;

   struct by_scope_owner;
   using compute_usage_index = chainbase::shared_multi_index_container<
      compute_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<compute_usage_object, compute_usage_object::id_type, &compute_usage_object::id>>,
         ordered_unique<tag<by_scope_owner>, 
            composite_key<compute_usage_object,
                member<compute_usage_object, scope_name,   &compute_usage_object::scope>,
                member<compute_usage_object, account_name, &compute_usage_object::owner>
            >
         >
      >
   >;


} } /// eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::bandwidth_usage_object, eosio::chain::bandwidth_usage_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::compute_usage_object, eosio::chain::compute_usage_index)
