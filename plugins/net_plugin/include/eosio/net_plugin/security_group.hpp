#pragma once
#include <eosio/chain/types.hpp>

#include <boost/container/flat_set.hpp>

#include <limits>
#include <atomic>

namespace eosio {
   /** \brief Manages the security group cache */
   class security_group_manager {
   public:
      security_group_manager() = default;
      security_group_manager(const security_group_manager&) = default;
      security_group_manager(security_group_manager&&) = delete;

      ~security_group_manager() = default;

      using participant_list_t = boost::container::flat_set<chain::account_name>;
      /** @brief Update the security group participants
       *
       * @param version             The version number for this update
       * @param participant_list    The list of accounts for the security group
       * @return    True if an update was performed.
       */
      bool update_cache(const uint32_t version, const participant_list_t& participant_list);
      /** @brief Determine if a participant is in the security group */
      bool is_in_security_group(chain::account_name participant) const { return cache_.find(participant) != cache_.end(); }
      security_group_manager& operator=(const security_group_manager&) = default;
      security_group_manager& operator=(security_group_manager&&) = delete;
   private:
      uint32_t version_ {std::numeric_limits<uint32_t>::max()}; ///!  The security group version
      participant_list_t cache_;    ///!    Cache of accounts in the security group
   };
}
