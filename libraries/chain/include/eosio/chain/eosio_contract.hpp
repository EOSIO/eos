#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>
#include <mutex>
namespace rodeos_testing {
   class timing {
   public:
      void received_block(uint32_t block_num, const fc::time_point& time) {
         std::lock_guard<std::mutex> g( received_mtx );
         received.emplace(block_num, std::make_pair(time, time));
      }

      void received_block_latest(uint32_t block_num, const fc::time_point& time) {
         std::lock_guard<std::mutex> g( received_mtx );
         received[block_num].second = fc::time_point::now();
      }

      std::pair<fc::time_point, fc::time_point> received_block(uint32_t block_num) {
         std::lock_guard<std::mutex> g( received_mtx );
         return received[block_num];
      }
      static timing* single() {
         static timing* singleton = nullptr;
         if (!singleton) {
            ilog("Creating rodeos_testing::timing (there should only be one)");
            singleton = new timing();
         }
         return singleton;
      }
   private:
      mutable std::mutex               received_mtx;
      std::unordered_map<uint32_t, std::pair<fc::time_point, fc::time_point> > received;
   };
}

namespace eosio { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_eosio_newaccount(apply_context&);
   void apply_eosio_updateauth(apply_context&);
   void apply_eosio_deleteauth(apply_context&);
   void apply_eosio_linkauth(apply_context&);
   void apply_eosio_unlinkauth(apply_context&);

   /*
   void apply_eosio_postrecovery(apply_context&);
   void apply_eosio_passrecovery(apply_context&);
   void apply_eosio_vetorecovery(apply_context&);
   */

   void apply_eosio_setcode(apply_context&);
   void apply_eosio_setabi(apply_context&);

   void apply_eosio_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
