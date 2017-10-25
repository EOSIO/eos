/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/transaction.hpp>

#include <random>
#include <set>

namespace eos { namespace chain {
   using pending_transaction = static_variant<std::reference_wrapper<const SignedTransaction>, std::reference_wrapper<const GeneratedTransaction>>;

   struct thread_schedule {
      vector<pending_transaction> transactions;
   };

   using cycle_schedule = vector<thread_schedule>;

   /**
    *   @class block_schedule
    *   @brief represents a proposed order of execution for a generated block
    */
   struct block_schedule
   {
      typedef block_schedule (*factory)(const vector<pending_transaction>&, const global_property_object&);
      vector<cycle_schedule> cycles;

      // Algorithms
      
      /**
       * A greedy scheduler that attempts to make short threads to resolve scope contention before 
       * falling back on cycles
       * @return the block scheduler
       */
      static block_schedule by_threading_conflicts(const vector<pending_transaction>& transactions, const global_property_object& properties);

      /**
       * A greedy scheduler that attempts uses future cycles to resolve scope contention
       * @return the block scheduler
       */
      static block_schedule by_cycling_conflicts(const vector<pending_transaction>& transactions, const global_property_object& properties);

      /**
       * A reference scheduler that puts all transactions in a single thread (FIFO)
       * @return the block scheduler
       */
      static block_schedule in_single_thread(const vector<pending_transaction>& transactions, const global_property_object& properties);
     
   };

   struct scope_extracting_visitor : public fc::visitor<std::set<AccountName>> {
      template <typename T>
      std::set<AccountName> operator()(std::reference_wrapper<const T> trx) const {
         const auto& t = trx.get();
         std::set<AccountName> unique_names(t.scope.begin(), t.scope.end());
         return unique_names;
      }
   };

} } // eos::chain

FC_REFLECT(eos::chain::thread_schedule, (transactions))
FC_REFLECT(eos::chain::block_schedule, (cycles))
