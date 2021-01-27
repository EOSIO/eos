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

   //order must match parameters as ids are used in serialization
   enum {
      max_block_net_usage_id,
      target_block_net_usage_pct_id,
      max_transaction_net_usage_id,
      base_per_transaction_net_usage_id,
      net_usage_leeway_id,
      context_free_discount_net_usage_num_id,
      context_free_discount_net_usage_den_id,
      max_block_cpu_usage_id,
      target_block_cpu_usage_pct_id,
      max_transaction_cpu_usage_id,
      min_transaction_cpu_usage_id,
      max_transaction_lifetime_id,
      deferred_trx_expiration_window_id,
      max_transaction_delay_id,
      max_inline_action_size_id,
      max_inline_action_depth_id,
      max_authority_depth_id,
      PARAMS_COUNT
   };

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
      return c.log(out) << "\n";
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

protected:
   template<typename Stream>
   Stream& log(Stream& out) const{
      return out << "Max Block Net Usage: " << max_block_net_usage << ", "
                     << "Target Block Net Usage Percent: " << ((double)target_block_net_usage_pct / (double)config::percent_1) << "%, "
                     << "Max Transaction Net Usage: " << max_transaction_net_usage << ", "
                     << "Base Per-Transaction Net Usage: " << base_per_transaction_net_usage << ", "
                     << "Net Usage Leeway: " << net_usage_leeway << ", "
                     << "Context-Free Data Net Usage Discount: " << (double)context_free_discount_net_usage_num * 100.0 / (double)context_free_discount_net_usage_den << "% , "

                     << "Max Block CPU Usage: " << max_block_cpu_usage << ", "
                     << "Target Block CPU Usage Percent: " << ((double)target_block_cpu_usage_pct / (double)config::percent_1) << "%, "
                     << "Max Transaction CPU Usage: " << max_transaction_cpu_usage << ", "
                     << "Min Transaction CPU Usage: " << min_transaction_cpu_usage << ", "

                     << "Max Transaction Lifetime: " << max_transaction_lifetime << ", "
                     << "Deferred Transaction Expiration Window: " << deferred_trx_expiration_window << ", "
                     << "Max Transaction Delay: " << max_transaction_delay << ", "
                     << "Max Inline Action Size: " << max_inline_action_size << ", "
                     << "Max Inline Action Depth: " << max_inline_action_depth << ", "
                     << "Max Authority Depth: " << max_authority_depth;
   }
};

/**
 * @brief v1 Producer-voted blockchain configuration parameters
 *
 * If Adding new parameters create chain_config_v[n] class instead of adding
 * new parameters to v1 or v0. This is needed for snapshots backward compatibility
 */
struct chain_config_v1 : chain_config_v0 {
   using Base = chain_config_v0;

   uint32_t   max_action_return_value_size = config::default_max_action_return_value_size;               ///< size limit for action return value
   
   //order must match parameters as ids are used in serialization
   enum {
     max_action_return_value_size_id = Base::PARAMS_COUNT,
     PARAMS_COUNT
   };

   inline const Base& base() const {
      return static_cast<const Base&>(*this);
   }

   void validate() const;

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config_v1& c ) {
      return c.log(out) << "\n";
   }

   friend inline bool operator == ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      //add v1 parameters comarison here
      return std::tie(lhs.max_action_return_value_size) == std::tie(rhs.max_action_return_value_size) 
          && lhs.base() == rhs.base();
   }

   friend inline bool operator != ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      return !(lhs == rhs);
   }

   inline chain_config_v1& operator= (const Base& b) {
      Base::operator= (b);
      return *this;
   }

protected:
   template<typename Stream>
   Stream& log(Stream& out) const{
      return base().log(out) << ", Max Action Return Value Size: " << max_action_return_value_size;
   }
};

class controller;

struct config_entry_validator{
   const controller& control;

   bool operator()(uint32_t id) const;
};

//after adding 1st value to chain_config_v1 change this using to point to v1
using chain_config = chain_config_v1;
using config_range = data_range<chain_config, config_entry_validator>;

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config_v0,
           (max_block_net_usage)(target_block_net_usage_pct)
           (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)
           (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

           (max_block_cpu_usage)(target_block_cpu_usage_pct)
           (max_transaction_cpu_usage)(min_transaction_cpu_usage)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

)

FC_REFLECT_DERIVED(eosio::chain::chain_config_v1, (eosio::chain::chain_config_v0), 
           (max_action_return_value_size)
)

namespace fc {

/**
 * @brief This is for packing data_entry<chain_config_v0, ...> 
 * that is used as part of packing data_range<chain_config_v0, ...>
 * @param s datastream
 * @param entry contains config reference and particular id
 * @throws config_parse_error if id is unknown
 */
template <typename DataStream>
inline DataStream &operator<<(DataStream &s, const eosio::chain::data_entry<eosio::chain::chain_config_v0, eosio::chain::config_entry_validator> &entry){
   using namespace eosio::chain;

   //initial requirements were to skip packing field if it is not activated.
   //this approach allows to spam this function with big buffer so changing this behavior
   EOS_ASSERT(entry.is_allowed(), unsupported_feature, "config id ${id} is no allowed", ("id", entry.id));
   
   switch (entry.id){
      case chain_config_v0::max_block_net_usage_id:
      fc::raw::pack(s, entry.config.max_block_net_usage);
      break;
      case chain_config_v0::target_block_net_usage_pct_id:
      fc::raw::pack(s, entry.config.target_block_net_usage_pct);
      break;
      case chain_config_v0::max_transaction_net_usage_id:
      fc::raw::pack(s, entry.config.max_transaction_net_usage);
      break;
      case chain_config_v0::base_per_transaction_net_usage_id:
      fc::raw::pack(s, entry.config.base_per_transaction_net_usage);
      break;
      case chain_config_v0::net_usage_leeway_id:
      fc::raw::pack(s, entry.config.net_usage_leeway);
      break;
      case chain_config_v0::context_free_discount_net_usage_num_id:
      fc::raw::pack(s, entry.config.context_free_discount_net_usage_num);
      break;
      case chain_config_v0::context_free_discount_net_usage_den_id:
      fc::raw::pack(s, entry.config.context_free_discount_net_usage_den);
      break;
      case chain_config_v0::max_block_cpu_usage_id:
      fc::raw::pack(s, entry.config.max_block_cpu_usage);
      break;
      case chain_config_v0::target_block_cpu_usage_pct_id:
      fc::raw::pack(s, entry.config.target_block_cpu_usage_pct);
      break;
      case chain_config_v0::max_transaction_cpu_usage_id:
      fc::raw::pack(s, entry.config.max_transaction_cpu_usage);
      break;
      case chain_config_v0::min_transaction_cpu_usage_id:
      fc::raw::pack(s, entry.config.min_transaction_cpu_usage);
      break;
      case chain_config_v0::max_transaction_lifetime_id:
      fc::raw::pack(s, entry.config.max_transaction_lifetime);
      break;
      case chain_config_v0::deferred_trx_expiration_window_id:
      fc::raw::pack(s, entry.config.deferred_trx_expiration_window);
      break;
      case chain_config_v0::max_transaction_delay_id:
      fc::raw::pack(s, entry.config.max_transaction_delay);
      break;
      case chain_config_v0::max_inline_action_size_id:
      fc::raw::pack(s, entry.config.max_inline_action_size);
      break;
      case chain_config_v0::max_inline_action_depth_id:
      fc::raw::pack(s, entry.config.max_inline_action_depth);
      break;
      case chain_config_v0::max_authority_depth_id:
      fc::raw::pack(s, entry.config.max_authority_depth);
      break;
      default:
      FC_THROW_EXCEPTION(config_parse_error, "DataStream& operator<<: no such id: ${id}", ("id", entry.id));
   }
   return s;
}

/**
 * @brief This is for packing data_entry<chain_config_v1, ...> 
 * that is used as part of packing data_range<chain_config_v1, ...>
 * @param s datastream
 * @param entry contains config reference and particular id
 * @throws unsupported_feature if protocol feature for particular id is not activated
 */
template <typename DataStream>
inline DataStream &operator<<(DataStream &s, const eosio::chain::data_entry<eosio::chain::chain_config_v1, eosio::chain::config_entry_validator> &entry){
   using namespace eosio::chain;

   //initial requirements were to skip packing field if it is not activated.
   //this approach allows to spam this function with big buffer so changing this behavior
   //moreover:
   //The contract has no way to know that the value was skipped and is likely to behave incorrectly.
   //When the protocol feature is not activated, the old version of nodeos that doesn't know about 
   //the entry MUST behave the same as the new version of nodeos that does.
   //Skipping known but unactivated entries violates this.
   EOS_ASSERT(entry.is_allowed(), unsupported_feature, "config id ${id} is no allowed", ("id", entry.id));
   
   switch (entry.id){
      case chain_config_v1::max_action_return_value_size_id:
      fc::raw::pack(s, entry.config.max_action_return_value_size);
      break;
      default:
      data_entry<chain_config_v0, config_entry_validator> base_entry(entry);
      fc::raw::pack(s, base_entry);
   }

   return s;
}

/**
 * @brief This is for unpacking data_entry<chain_config_v0, ...> 
 * that is used as part of unpacking data_range<chain_config_v0, ...>
 * @param s datastream
 * @param entry contains config reference and particular id
 * @throws unsupported_feature if protocol feature for particular id is not activated
 */
template <typename DataStream>
inline DataStream &operator>>(DataStream &s, eosio::chain::data_entry<eosio::chain::chain_config_v0, eosio::chain::config_entry_validator> &entry){
   using namespace eosio::chain;

   EOS_ASSERT(entry.is_allowed(), eosio::chain::unsupported_feature, "config id ${id} is no allowed", ("id", entry.id));

   switch (entry.id){
      case chain_config_v0::max_block_net_usage_id:
      fc::raw::unpack(s, entry.config.max_block_net_usage);
      break;
      case chain_config_v0::target_block_net_usage_pct_id:
      fc::raw::unpack(s, entry.config.target_block_net_usage_pct);
      break;
      case chain_config_v0::max_transaction_net_usage_id:
      fc::raw::unpack(s, entry.config.max_transaction_net_usage);
      break;
      case chain_config_v0::base_per_transaction_net_usage_id:
      fc::raw::unpack(s, entry.config.base_per_transaction_net_usage);
      break;
      case chain_config_v0::net_usage_leeway_id:
      fc::raw::unpack(s, entry.config.net_usage_leeway);
      break;
      case chain_config_v0::context_free_discount_net_usage_num_id:
      fc::raw::unpack(s, entry.config.context_free_discount_net_usage_num);
      break;
      case chain_config_v0::context_free_discount_net_usage_den_id:
      fc::raw::unpack(s, entry.config.context_free_discount_net_usage_den);
      break;
      case chain_config_v0::max_block_cpu_usage_id:
      fc::raw::unpack(s, entry.config.max_block_cpu_usage);
      break;
      case chain_config_v0::target_block_cpu_usage_pct_id:
      fc::raw::unpack(s, entry.config.target_block_cpu_usage_pct);
      break;
      case chain_config_v0::max_transaction_cpu_usage_id:
      fc::raw::unpack(s, entry.config.max_transaction_cpu_usage);
      break;
      case chain_config_v0::min_transaction_cpu_usage_id:
      fc::raw::unpack(s, entry.config.min_transaction_cpu_usage);
      break;
      case chain_config_v0::max_transaction_lifetime_id:
      fc::raw::unpack(s, entry.config.max_transaction_lifetime);
      break;
      case chain_config_v0::deferred_trx_expiration_window_id:
      fc::raw::unpack(s, entry.config.deferred_trx_expiration_window);
      break;
      case chain_config_v0::max_transaction_delay_id:
      fc::raw::unpack(s, entry.config.max_transaction_delay);
      break;
      case chain_config_v0::max_inline_action_size_id:
      fc::raw::unpack(s, entry.config.max_inline_action_size);
      break;
      case chain_config_v0::max_inline_action_depth_id:
      fc::raw::unpack(s, entry.config.max_inline_action_depth);
      break;
      case chain_config_v0::max_authority_depth_id:
      fc::raw::unpack(s, entry.config.max_authority_depth);
      break;
      default:
      FC_THROW_EXCEPTION(eosio::chain::config_parse_error, "DataStream& operator<<: no such id: ${id}", ("id", entry.id));
   }
   
   return s;
}

/**
 * @brief This is for unpacking data_entry<chain_config_v1, ...> 
 * that is used as part of unpacking data_range<chain_config_v1, ...>
 * @param s datastream
 * @param entry contains config reference and particular id
 * @throws unsupported_feature if protocol feature for particular id is not activated
 */
template <typename DataStream>
inline DataStream &operator>>(DataStream &s, eosio::chain::data_entry<eosio::chain::chain_config_v1, eosio::chain::config_entry_validator> &entry){
   using namespace eosio::chain;

   EOS_ASSERT(entry.is_allowed(), unsupported_feature, "config id ${id} is no allowed", ("id", entry.id));

   switch (entry.id){
      case chain_config_v1::max_action_return_value_size_id:
      fc::raw::unpack(s, entry.config.max_action_return_value_size);
      break;
      default:
      eosio::chain::data_entry<chain_config_v0, config_entry_validator> base_entry(entry);
      fc::raw::unpack(s, base_entry);
   }

   return s;
}

/**
 * @brief Packs config stream in the following format:
 * |uint32_t:sequence_length | uint32_t:parameter_id | <various>:parameter_value | ... 
 * @param s datastream
 * @param selection contains ids range to pack
 * @throws config_parse_error on duplicate or unknown id in selection
 */
template<typename DataStream, typename T>
inline DataStream& operator<<( DataStream& s, const eosio::chain::data_range<T, eosio::chain::config_entry_validator>& selection ) {
   using namespace eosio::chain;
   
   fc::unsigned_int size = selection.ids.size();
   fc::raw::pack(s, size);

   //vector here serves as hash map where key is always an index
   std::vector<bool> visited(T::PARAMS_COUNT, false);
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

/**
 * @brief Unpacks config stream in the following format:
 * |uint32_t:sequence_length | uint32_t:parameter_id | <various>:parameter_value | ... 
 * @param s datastream
 * @param selection contains config reference where values will be unpacked
 * @throws config_parse_error on duplicate or unknown id in stream
 */
template<typename DataStream, typename T>
inline DataStream& operator>>( DataStream& s, eosio::chain::data_range<T, eosio::chain::config_entry_validator>& selection ) {
   using namespace eosio::chain;
   
   fc::unsigned_int length;
   fc::raw::unpack(s, length);

   //vector here serves as hash map where key is always an index
   std::vector<bool> visited(T::PARAMS_COUNT, false);
   for (uint32_t i = 0; i < length; ++i) {
      fc::unsigned_int id;
      fc::raw::unpack(s, id);
      
      EOS_ASSERT(id.value < visited.size(), config_parse_error, "provided id ${id} should be less than ${size}", ("id", id)("size", visited.size()));
      EOS_ASSERT(!visited[id], config_parse_error, "duplicate id provided: ${id}", ("id", id));
      visited[id] = true;

      data_entry<T, config_entry_validator> cfg_entry(selection.config, id, selection.validator);
      fc::raw::unpack(s, cfg_entry);
   }
   return s;
}

} //namespace fc