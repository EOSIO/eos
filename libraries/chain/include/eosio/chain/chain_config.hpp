/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>

namespace eosio { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config {
   uint32_t   base_per_transaction_net_usage;      ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   context_free_discount_net_usage_num; ///< the numerator for the discount on net usage of context-free data
   uint32_t   context_free_discount_net_usage_den; ///< the denominator for the discount on net usage of context-free data

   uint32_t   min_transaction_cpu_usage;           ///< the minimum billable cpu usage (in microseconds) that the chain requires

   uint64_t   min_transaction_ram_usage;           ///< the minimum billable ram usage (in bytes) that the chain requires

   uint32_t   max_transaction_lifetime;            ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
   uint32_t   deferred_trx_expiration_window;      ///< the number of seconds after the time a deferred transaction can first execute until it expires
   uint32_t   max_transaction_delay;               ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
   uint32_t   max_inline_action_size;              ///< maximum allowed size (in bytes) of an inline action
   uint16_t   max_inline_action_depth;             ///< recursion depth limit on sending inline actions
   uint16_t   max_authority_depth;                 ///< recursion depth limit for checking if an authority is satisfied

   uint64_t   ram_size;
   uint64_t   reserved_ram_size;
   
   std::vector<uint64_t> max_block_usage;
   std::vector<uint64_t> max_transaction_usage;
   
   std::vector<uint64_t> target_virtual_limits;
   std::vector<uint64_t> min_virtual_limits;
   std::vector<uint64_t> max_virtual_limits;
   std::vector<uint32_t> usage_windows;
   
   std::vector<uint16_t> virtual_limit_decrease_pct;
   std::vector<uint16_t> virtual_limit_increase_pct;

   std::vector<uint32_t> account_usage_windows;

   void validate()const;

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config& c ) {
      out 
         << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "
         << "Context-Free Data Net Usage Discount: " << (double)c.context_free_discount_net_usage_num * 100.0 / (double)c.context_free_discount_net_usage_den << "% , "

         << "Min Transaction CPU Usage: " << c.min_transaction_cpu_usage << ", "

         << "Min Transaction RAM Usage: " << c.min_transaction_ram_usage << ", "

         << "Max Transaction Lifetime: " << c.max_transaction_lifetime << ", "
         << "Deferred Transaction Expiration Window: " << c.deferred_trx_expiration_window << ", "
         << "Max Transaction Delay: " << c.max_transaction_delay << ", "
         << "Max Inline Action Size: " << c.max_inline_action_size << ", "
         << "Max Inline Action Depth: " << c.max_inline_action_depth << ", "
         << "Max Authority Depth: " << c.max_authority_depth << ", "
         << "RAM size: " << c.ram_size << ", "
         << "Reserved RAM size: " << c.reserved_ram_size << "\n";

      out << "Max Block Usage: ";            for (auto r : c.max_block_usage)            { out << r << " "; } out << "\n";
      out << "Max Transaction Usage: ";      for (auto r : c.max_transaction_usage)      { out << r << " "; } out << "\n";
      out << "Target Virtual Limits: ";      for (auto r : c.target_virtual_limits)      { out << r << " "; } out << "\n";
      out << "Min Virtual Limits: ";         for (auto r : c.min_virtual_limits)         { out << r << " "; } out << "\n";
      out << "Max Virtual Limits: ";         for (auto r : c.max_virtual_limits)         { out << r << " "; } out << "\n";
      out << "Usage Windows: ";              for (auto r : c.usage_windows)              { out << r << " "; } out << "\n";
      out << "Virtual Limit Decrease pct: "; for (auto r : c.virtual_limit_decrease_pct) { out << r << " "; } out << "\n";
      out << "Virtual Limit Increase pct: "; for (auto r : c.virtual_limit_increase_pct) { out << r << " "; } out << "\n";
      out << "Account Usage Windows: ";      for (auto r : c.account_usage_windows)      { out << r << " "; } out << "\n";
   
      return out;
   }

   friend inline bool operator ==( const chain_config& lhs, const chain_config& rhs ) {
      return   std::tie(   lhs.base_per_transaction_net_usage,
                           lhs.context_free_discount_net_usage_num,
                           lhs.context_free_discount_net_usage_den,
                           lhs.min_transaction_cpu_usage,
                           lhs.min_transaction_ram_usage,
                           lhs.max_transaction_lifetime,
                           lhs.deferred_trx_expiration_window,
                           lhs.max_transaction_delay,
                           lhs.max_inline_action_size,
                           lhs.max_inline_action_depth,
                           lhs.max_authority_depth,
                           lhs.ram_size,
                           lhs.reserved_ram_size,
                           lhs.max_block_usage,
                           lhs.max_transaction_usage,
                           lhs.target_virtual_limits,
                           lhs.min_virtual_limits,
                           lhs.max_virtual_limits,
                           lhs.usage_windows,
                           lhs.virtual_limit_decrease_pct,
                           lhs.virtual_limit_increase_pct,
                           lhs.account_usage_windows
                        )
               ==
               std::tie(   rhs.base_per_transaction_net_usage,
                           rhs.context_free_discount_net_usage_num,
                           rhs.context_free_discount_net_usage_den,
                           rhs.min_transaction_cpu_usage,
                           rhs.min_transaction_ram_usage,
                           rhs.max_transaction_lifetime,
                           rhs.deferred_trx_expiration_window,
                           rhs.max_transaction_delay,
                           rhs.max_inline_action_size,
                           rhs.max_inline_action_depth,
                           rhs.max_authority_depth,
                           rhs.ram_size,
                           rhs.reserved_ram_size,
                           rhs.max_block_usage,
                           rhs.max_transaction_usage,
                           rhs.target_virtual_limits,
                           rhs.min_virtual_limits,
                           rhs.max_virtual_limits,
                           rhs.usage_windows,
                           rhs.virtual_limit_decrease_pct,
                           rhs.virtual_limit_increase_pct,
                           rhs.account_usage_windows
                        );
   };

   friend inline bool operator !=( const chain_config& lhs, const chain_config& rhs ) { return !(lhs == rhs); }

};

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config,
           (base_per_transaction_net_usage)
           (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

           (min_transaction_cpu_usage)
           (min_transaction_ram_usage)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

           (ram_size)(reserved_ram_size)
           (max_block_usage)(max_transaction_usage)
           
           (target_virtual_limits)(min_virtual_limits)(max_virtual_limits)(usage_windows)
           (virtual_limit_decrease_pct)(virtual_limit_increase_pct)(account_usage_windows)

)
