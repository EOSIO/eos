#pragma once
#include <eosio/chain/types.hpp>

#include <boost/container/flat_set.hpp>

#include <limits>
#include <atomic>

namespace eosio {
   /** \brief Describes a security group participant */
   class security_group_participant {
   public:
      /** \brief  Simple initialization
       *
       * @param account_name    The recognized name of the participant
       * @param participating   Indicates the participant's participation (default = true)
       *                            true - participating
       *                            false - sync'ing only
       */
      explicit security_group_participant(chain::account_name account_name, bool participating = true) :
         participant_(account_name), participating_(participating) {}
      security_group_participant(const security_group_participant& that) :
         participant_(that.participant_), participating_(that.is_participating()) {}
      ~security_group_participant() = default;

      auto name() const { return participant_; }

      [[maybe_unused]]
      bool is_participating() const { return participating_.load(std::memory_order_relaxed); }
      bool is_syncing() const { return !participating_.load(std::memory_order_relaxed); }
      void now_syncing() { participating_.store(false, std::memory_order_relaxed); }
      void now_participating() { participating_.store(true, std::memory_order_relaxed); }

      security_group_participant& operator=(const security_group_participant& that) {
         participant_ = that.participant_;
         participating_.store(that.is_participating(), std::memory_order_relaxed);
         return *this;
      }

      bool operator==(const security_group_participant& that) const { return that.participant_ == participant_; }
      bool operator!=(const security_group_participant& that) const { return !operator==(that); }
      bool operator<(const security_group_participant& that) const { return participant_ < that.participant_; }
      bool operator>(const security_group_participant& that) const { return participant_ > that.participant_; }

      bool operator==(chain::account_name account_name) const { return account_name == participant_; }

   private:
      chain::account_name participant_;            //!<  Identifier for this participant
      std::atomic<bool> participating_ {true};  //!<  Indicates if participant producing blocks (true) or
                                                   //!<  only used for sync'ing
   };

   /** \brief Describes the security group cache */
   class security_group_manager {
   public:
      security_group_manager() = default;
      security_group_manager(const security_group_manager&) = delete;
      security_group_manager(security_group_manager&&) = delete;

      ~security_group_manager() = default;

      using account_name_list_t = boost::container::flat_set<chain::account_name>;
      using callback_t = std::function<void(chain::account_name)>;
      /** Update the cache using new information from a block
       *
       * @param version             The version number for this update
       * @param participant_list    The list of accounts for the security group
       * @param on_participating    Callback when an account is added to the cache
       * @param on_syncing          Callback when an account is "removed" from the cache
       */
      void update_cache(const uint32_t version, const account_name_list_t& participant_list, callback_t on_participating,
                        callback_t on_syncing);

      using cache_t = boost::container::flat_set<security_group_participant>;
      const cache_t& security_group() const { return cache_; }

      security_group_manager& operator==(const security_group_manager&) = delete;
      security_group_manager& operator==(security_group_manager&&) = delete;
   private:
      uint32_t version_ {std::numeric_limits<uint32_t>::max()}; ///!  The security group version
      cache_t cache_;   ///!    Cache of accounts in the security group
   };
}
