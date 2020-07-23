#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/chain_config_helper.hpp>

namespace eosio { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config_v0 {
   uint64_t   max_block_net_usage;                 ///< the maxiumum net usage in instructions for a block
   uint32_t   target_block_net_usage_pct;          ///< the target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling
   uint32_t   max_transaction_net_usage;           ///< the maximum objectively measured net usage that the chain will allow regardless of account limits
   uint32_t   base_per_transaction_net_usage;      ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   net_usage_leeway;
   uint32_t   context_free_discount_net_usage_num; ///< the numerator for the discount on net usage of context-free data
   uint32_t   context_free_discount_net_usage_den; ///< the denominator for the discount on net usage of context-free data

   uint32_t   max_block_cpu_usage;                 ///< the maxiumum billable cpu usage (in microseconds) for a block
   uint32_t   target_block_cpu_usage_pct;          ///< the target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
   uint32_t   max_transaction_cpu_usage;           ///< the maximum billable cpu usage (in microseconds) that the chain will allow regardless of account limits
   uint32_t   min_transaction_cpu_usage;           ///< the minimum billable cpu usage (in microseconds) that the chain requires

   uint32_t   max_transaction_lifetime;            ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
   uint32_t   deferred_trx_expiration_window;      ///< the number of seconds after the time a deferred transaction can first execute until it expires
   uint32_t   max_transaction_delay;               ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
   uint32_t   max_inline_action_size;              ///< maximum allowed size (in bytes) of an inline action
   uint16_t   max_inline_action_depth;             ///< recursion depth limit on sending inline actions
   uint16_t   max_authority_depth;                 ///< recursion depth limit for checking if an authority is satisfied

   void validate()const;

   inline const chain_config_v0& v0() const {
      return *this;
   }
   
   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config_v0& c ) {
      return out << "Max Block Net Usage: " << c.max_block_net_usage << ", "
                 << "Target Block Net Usage Percent: " << ((double)c.target_block_net_usage_pct / (double)config::percent_1) << "%, "
                 << "Max Transaction Net Usage: " << c.max_transaction_net_usage << ", "
                 << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "
                 << "Net Usage Leeway: " << c.net_usage_leeway << ", "
                 << "Context-Free Data Net Usage Discount: " << (double)c.context_free_discount_net_usage_num * 100.0 / (double)c.context_free_discount_net_usage_den << "% , "

                 << "Max Block CPU Usage: " << c.max_block_cpu_usage << ", "
                 << "Target Block CPU Usage Percent: " << ((double)c.target_block_cpu_usage_pct / (double)config::percent_1) << "%, "
                 << "Max Transaction CPU Usage: " << c.max_transaction_cpu_usage << ", "
                 << "Min Transaction CPU Usage: " << c.min_transaction_cpu_usage << ", "

                 << "Max Transaction Lifetime: " << c.max_transaction_lifetime << ", "
                 << "Deferred Transaction Expiration Window: " << c.deferred_trx_expiration_window << ", "
                 << "Max Transaction Delay: " << c.max_transaction_delay << ", "
                 << "Max Inline Action Size: " << c.max_inline_action_size << ", "
                 << "Max Inline Action Depth: " << c.max_inline_action_depth << ", "
                 << "Max Authority Depth: " << c.max_authority_depth << "\n";
   }

   friend inline bool operator ==( const chain_config_v0& lhs, const chain_config_v0& rhs ) {
      return   std::tie(   lhs.max_block_net_usage,
                           lhs.target_block_net_usage_pct,
                           lhs.max_transaction_net_usage,
                           lhs.base_per_transaction_net_usage,
                           lhs.net_usage_leeway,
                           lhs.context_free_discount_net_usage_num,
                           lhs.context_free_discount_net_usage_den,
                           lhs.max_block_cpu_usage,
                           lhs.target_block_cpu_usage_pct,
                           lhs.max_transaction_cpu_usage,
                           lhs.max_transaction_cpu_usage,
                           lhs.max_transaction_lifetime,
                           lhs.deferred_trx_expiration_window,
                           lhs.max_transaction_delay,
                           lhs.max_inline_action_size,
                           lhs.max_inline_action_depth,
                           lhs.max_authority_depth
                        )
               ==
               std::tie(   rhs.max_block_net_usage,
                           rhs.target_block_net_usage_pct,
                           rhs.max_transaction_net_usage,
                           rhs.base_per_transaction_net_usage,
                           rhs.net_usage_leeway,
                           rhs.context_free_discount_net_usage_num,
                           rhs.context_free_discount_net_usage_den,
                           rhs.max_block_cpu_usage,
                           rhs.target_block_cpu_usage_pct,
                           rhs.max_transaction_cpu_usage,
                           rhs.max_transaction_cpu_usage,
                           rhs.max_transaction_lifetime,
                           rhs.deferred_trx_expiration_window,
                           rhs.max_transaction_delay,
                           rhs.max_inline_action_size,
                           rhs.max_inline_action_depth,
                           rhs.max_authority_depth
                        );
   };

   friend inline bool operator !=( const chain_config_v0& lhs, const chain_config_v0& rhs ) { return !(lhs == rhs); }

};

struct chain_config_v1 : chain_config_v0 {
   using Base = chain_config_v0;
   //add parameters here:

   inline const Base& base() const {
      return static_cast<const Base&>(*this);
   }

   void validate() const;

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config_v1& c ) {
      //add v1 parameters output here
      return out << c.base();
   }

   friend inline bool operator == ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      //add v1 parameters comarison here
      return lhs.base() == rhs.base();
   }

   friend inline bool operator != ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      return !(lhs == rhs);
   }

   inline chain_config_v1& operator= (const Base& b) {
      Base::operator= (b);
      return *this;
   }
};

class controller;

struct config_entry_validator{
   const controller& control;

   bool operator()(uint32_t id) const;
};

//after adding 1st value to chain_config_v1 change this using to point to v1
using chain_config = chain_config_v0;
using config_range = data_range<chain_config, config_entry_validator>;

#define CHAIN_CONFIG_V0_MEMBERS()\
            (max_block_net_usage)(target_block_net_usage_pct)\
            (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)\
            (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)\
            (max_block_cpu_usage)(target_block_cpu_usage_pct)\
            (max_transaction_cpu_usage)(min_transaction_cpu_usage)\
            (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)\
            (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

DEFINE_ENUM(chain_config_v0, CHAIN_CONFIG_V0_MEMBERS())

//add new v1 members here. order is important
#define CHAIN_CONFIG_V1_MEMBERS()\
            ()

//uncomment after adding 1st member to v1 config
//DEFINE_ENUM_DERIVED(chain_config_v0, chain_config_v1, CHAIN_CONFIG_V1_MEMBERS())

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config_v0, CHAIN_CONFIG_V0_MEMBERS())
//uncomment after adding 1st member to v1 config
//FC_REFLECT_DERIVED(eosio::chain::chain_config_v1, (eosio::chain::chain_config_v0), CHAIN_CONFIG_V1_MEMBERS())


namespace fc {

RANGE_PACK(eosio::chain::chain_config_v0, CHAIN_CONFIG_V0_MEMBERS())
RANGE_UNPACK(eosio::chain::chain_config_v0, CHAIN_CONFIG_V0_MEMBERS())
//uncomment after adding 1st member to v1 config
//RANGE_PACK(eosio::chain::chain_config_v1, CHAIN_CONFIG_V1_MEMBERS())
//RANGE_UNPACK(eosio::chain::chain_config_v1, CHAIN_CONFIG_V1_MEMBERS())


/**
 * Packed config stream is in the following format:
 * |uint32_t:sequence_length | uint32_t:parameter_id | <various>:parameter_value | ... 
 * all parameters are optional
 */
template<typename DataStream, typename T>
inline DataStream& operator<<( DataStream& s, const eosio::chain::data_range<T, eosio::chain::config_entry_validator>& selection ) {
   using namespace eosio::chain;
   
   fc::unsigned_int size = selection.ids.size();
   fc::raw::pack(s, size);

   //vector here serves as hash map where key is always an index
   std::vector<bool> visited(enum_size<T>(), false);
   for (auto uid : selection.ids){
      uint32_t id = uid;
      EOS_ASSERT(id < visited.size(), config_parse_error, "provided id ${id} should be less than ${size}", ("id", id)("size", visited.size()));
      EOS_ASSERT(!visited[id], config_parse_error, "duplicate id provided: ${id}", ("id", id));
      visited[id] = true;

      fc::raw::pack(s, fc::unsigned_int(id));
      fc::raw::pack(s, data_entry(selection.config, id, selection.validator));
   }

   return s;
}

template<typename DataStream, typename T>
inline DataStream& operator>>( DataStream& s, eosio::chain::data_range<T, eosio::chain::config_entry_validator>& selection ) {
   using namespace eosio::chain;
   
   fc::unsigned_int length;
   fc::raw::unpack(s, length);

   //vector here serves as hash map where key is always an index
   std::vector<bool> visited(enum_size<T>(), false);
   for (uint32_t i = 0; i < length; ++i) {
      fc::unsigned_int id;
      fc::raw::unpack(s, id);
      
      EOS_ASSERT(id.value < visited.size(), config_parse_error, "provided id ${id} should be less than ${size}", ("id", id)("size", visited.size()));
      EOS_ASSERT(!visited[id], config_parse_error, "duplicate id provided: ${id}", ("id", id));
      visited[id] = true;

      eosio::chain::data_entry<T, config_entry_validator> cfg_entry(selection.config, id, selection.validator);
      fc::raw::unpack(s, cfg_entry);
   }
   return s;
}

} //namespace fc