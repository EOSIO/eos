#include <eosio/net_plugin/security_group.hpp>

namespace eosio {
   // Cache update looks at 3 scenarios:
   //    1) The cache is empty and is being populated
   //    2) The update is empty so all accounts are being removed from the cache
   //    3) The cache is populated and accouts are being added and/or removed
   void security_group_manager::update_cache(const uint32_t version, const account_name_list_t& participant_list,
                                             callback_t on_participating, callback_t on_syncing) {
      // update on version change
      if(version == version_)
         return;
      version_ = version;

      // Scenario 1: The cache is empty and being populated
      //
      if(cache_.empty()) {
         if(participant_list.empty()) {
            return;
         }
         for(auto participant : participant_list) {
            cache_.emplace(participant);
            on_participating(participant);
         }
         return;
      }

      // Scenaio 2: The update is empty so all accounts are being removed from the cache
      //
      if(participant_list.empty()) {
         for(auto& participant : cache_) {
            if(participant.is_participating()) {
               participant.now_syncing();
               on_syncing(participant.name());
            }
         }
         return;
      }

      // Scenario 3: Accounts are added and/or removed from cache
      //
      //    1) Update status of existing accounts
      //    2) Add new accounts
      //
      for(auto& participant : cache_) {
         if(participant_list.find(participant.name()) != participant_list.end()) {
            if(participant.is_syncing()) {
               participant.now_participating();
               on_participating(participant.name());
            }
         }
         else {
            participant.now_syncing();
            on_syncing(participant.name());
         }
      }

      for(const auto& participant : participant_list) {
         if(cache_.emplace(participant).second) {
            on_participating(participant);
         }
      }
   }
}
