#pragma once
#include <eosio/chain/types.hpp>

#include <boost/container/flat_set.hpp>

#include <atomic>

namespace eosio {
   /** \brief Manages the security group cache */
   class security_group_manager {
   public:
      using participant_list_t = boost::container::flat_set<chain::account_name>;
      /** @brief Provides the current security group version */
      auto current_version() const { return version_; }
      /** @brief Update the security group participants
       *
       * @param version             The version number for this update
       * @param participant_list    The list of accounts for the security group
       * @return True if an update was performed.
       */
      bool update_cache(const uint32_t version, const participant_list_t& participant_list);
      /** @brief Determine if a participant is in the security group */
      bool is_in_security_group(chain::account_name participant) const {
         return cache_.empty() || (cache_.find(participant) != cache_.end());
      }
   private:
      uint32_t version_ {0};        ///! The security group version
      participant_list_t cache_;    ///! Cache of participants in the security group
   };
}
