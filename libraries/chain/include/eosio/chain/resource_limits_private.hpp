#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>

#include "multi_index_includes.hpp"


namespace eosio { namespace chain { namespace resource_limits {

   namespace impl {
      template<typename T>
      ratio<T> make_ratio(T n, T d) {
         return ratio<T>{n, d};
      }

      template<typename T>
      T operator* (T value, const ratio<T>& r) {
         EOS_ASSERT(r.numerator == T(0) || std::numeric_limits<T>::max() / r.numerator >= value, rate_limiting_state_inconsistent, "Usage exceeds maximum value representable after extending for precision");
         return (value * r.numerator) / r.denominator;
      }

      template<typename UnsignedIntType>
      constexpr UnsignedIntType integer_divide_ceil(UnsignedIntType num, UnsignedIntType den ) {
         return (num / den) + ((num % den) > 0 ? 1 : 0);
      }


      template<typename LesserIntType, typename GreaterIntType>
      constexpr bool is_valid_downgrade_cast =
         std::is_integral<LesserIntType>::value &&  // remove overloads where type is not integral
         std::is_integral<GreaterIntType>::value && // remove overloads where type is not integral
         (std::numeric_limits<LesserIntType>::max() <= std::numeric_limits<GreaterIntType>::max()); // remove overloads which are upgrades not downgrades

      /**
       * Specialization for Signedness matching integer types
       */
      template<typename LesserIntType, typename GreaterIntType>
      constexpr auto downgrade_cast(GreaterIntType val) ->
         std::enable_if_t<is_valid_downgrade_cast<LesserIntType,GreaterIntType> && std::is_signed<LesserIntType>::value == std::is_signed<GreaterIntType>::value, LesserIntType>
      {
         const GreaterIntType max = std::numeric_limits<LesserIntType>::max();
         const GreaterIntType min = std::numeric_limits<LesserIntType>::min();
         EOS_ASSERT( val >= min && val <= max, rate_limiting_state_inconsistent, "Casting a higher bit integer value ${v} to a lower bit integer value which cannot contain the value, valid range is [${min}, ${max}]", ("v", val)("min", min)("max",max) );
         return LesserIntType(val);
      };

      /**
       * Specialization for Signedness mismatching integer types
       */
      template<typename LesserIntType, typename GreaterIntType>
      constexpr auto downgrade_cast(GreaterIntType val) ->
         std::enable_if_t<is_valid_downgrade_cast<LesserIntType,GreaterIntType> && std::is_signed<LesserIntType>::value != std::is_signed<GreaterIntType>::value, LesserIntType>
      {
         const GreaterIntType max = std::numeric_limits<LesserIntType>::max();
         const GreaterIntType min = 0;
         EOS_ASSERT( val >= min && val <= max, rate_limiting_state_inconsistent, "Casting a higher bit integer value ${v} to a lower bit integer value which cannot contain the value, valid range is [${min}, ${max}]", ("v", val)("min", min)("max",max) );
         return LesserIntType(val);
      };

      /**
       *  This class accumulates and exponential moving average based on inputs
       *  This accumulator assumes there are no drops in input data
       *
       *  The value stored is Precision times the sum of the inputs.
       */
      template<uint64_t Precision = config::rate_limiting_precision>
      struct exponential_moving_average_accumulator
      {
         static_assert( Precision > 0, "Precision must be positive" );
         static constexpr uint64_t max_raw_value = std::numeric_limits<uint64_t>::max() / Precision;

         exponential_moving_average_accumulator()
         : last_ordinal(0)
         , value_ex(0)
         , consumed(0)
         {
         }

         uint32_t   last_ordinal;  ///< The ordinal of the last period which has contributed to the average
         uint64_t   value_ex;      ///< The current average pre-multiplied by Precision
         uint64_t   consumed;       ///< The last periods average + the current periods contribution so far

         /**
          * return the average value
          */
         uint64_t average() const {
            return integer_divide_ceil(value_ex, Precision);
         }

         void add( uint64_t units, uint32_t ordinal, uint32_t window_size /* must be positive */ )
         {
            // check for some numerical limits before doing any state mutations
            EOS_ASSERT(units <= max_raw_value, rate_limiting_state_inconsistent, "Usage exceeds maximum value representable after extending for precision");
            EOS_ASSERT(std::numeric_limits<decltype(consumed)>::max() - consumed >= units, rate_limiting_state_inconsistent, "Overflow in tracked usage when adding usage!");

            auto value_ex_contrib = downgrade_cast<uint64_t>(integer_divide_ceil((uint128_t)units * Precision, (uint128_t)window_size));
            EOS_ASSERT(std::numeric_limits<decltype(value_ex)>::max() - value_ex >= value_ex_contrib, rate_limiting_state_inconsistent, "Overflow in accumulated value when adding usage!");

            if( last_ordinal != ordinal ) {
               FC_ASSERT( ordinal > last_ordinal, "new ordinal cannot be less than the previous ordinal" );
               if( (uint64_t)last_ordinal + window_size > (uint64_t)ordinal ) {
                  const auto delta = ordinal - last_ordinal; // clearly 0 < delta < window_size
                  const auto decay = make_ratio(
                          (uint64_t)window_size - delta,
                          (uint64_t)window_size
                  );

                  value_ex = value_ex * decay;
               } else {
                  value_ex = 0;
               }

               last_ordinal = ordinal;
               consumed = average();
            }

            consumed += units;
            value_ex += value_ex_contrib;
         }
      };

   }

   using usage_accumulator = impl::exponential_moving_average_accumulator<>;

   /**
    * Every account that authorizes a transaction is billed for the full size of that transaction. This object
    * tracks the average usage of that account.
    */
   struct resource_limits_object : public chainbase::object<resource_limits_object_type, resource_limits_object> {

      OBJECT_CTOR(resource_limits_object)

      id_type id;
      account_name owner;
      bool pending = false;

      int64_t net_weight = -1;
      int64_t cpu_weight = -1;
      int64_t ram_bytes = -1;

   };

   struct by_owner;
   struct by_dirty;

   using resource_limits_index = chainbase::shared_multi_index_container<
      resource_limits_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_limits_object, resource_limits_object::id_type, &resource_limits_object::id>>,
         ordered_unique<tag<by_owner>,
            composite_key<resource_limits_object,
               BOOST_MULTI_INDEX_MEMBER(resource_limits_object, bool, pending),
               BOOST_MULTI_INDEX_MEMBER(resource_limits_object, account_name, owner)
            >
         >
      >
   >;

   struct resource_usage_object : public chainbase::object<resource_usage_object_type, resource_usage_object> {
      OBJECT_CTOR(resource_usage_object)

      id_type id;
      account_name owner;

      usage_accumulator        net_usage;
      usage_accumulator        cpu_usage;

      uint64_t                 ram_usage = 0;
   };

   using resource_usage_index = chainbase::shared_multi_index_container<
      resource_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_usage_object, resource_usage_object::id_type, &resource_usage_object::id>>,
         ordered_unique<tag<by_owner>, member<resource_usage_object, account_name, &resource_usage_object::owner> >
      >
   >;

   class resource_limits_config_object : public chainbase::object<resource_limits_config_object_type, resource_limits_config_object> {
      OBJECT_CTOR(resource_limits_config_object);
      id_type id;

      static_assert( config::block_interval_ms > 0, "config::block_interval_ms must be positive" );
      static_assert( config::block_cpu_usage_average_window_ms >= config::block_interval_ms,
                     "config::block_cpu_usage_average_window_ms cannot be less than config::block_interval_ms" );
      static_assert( config::block_size_average_window_ms >= config::block_interval_ms,
                     "config::block_size_average_window_ms cannot be less than config::block_interval_ms" );


      elastic_limit_parameters cpu_limit_parameters = {EOS_PERCENT(config::default_max_block_cpu_usage, config::default_target_block_cpu_usage_pct), config::default_max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}};
      elastic_limit_parameters net_limit_parameters = {EOS_PERCENT(config::default_max_block_net_usage, config::default_target_block_net_usage_pct), config::default_max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}};

      uint32_t account_cpu_usage_average_window = config::account_cpu_usage_average_window_ms / config::block_interval_ms;
      uint32_t account_net_usage_average_window = config::account_net_usage_average_window_ms / config::block_interval_ms;
   };

   using resource_limits_config_index = chainbase::shared_multi_index_container<
      resource_limits_config_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_limits_config_object, resource_limits_config_object::id_type, &resource_limits_config_object::id>>
      >
   >;

   class resource_limits_state_object : public chainbase::object<resource_limits_state_object_type, resource_limits_state_object> {
      OBJECT_CTOR(resource_limits_state_object);
      id_type id;

      /**
       * Track the average netusage for blocks
       */
      usage_accumulator average_block_net_usage;

      /**
       * Track the average cpu usage for blocks
       */
      usage_accumulator average_block_cpu_usage;

      void update_virtual_net_limit( const resource_limits_config_object& cfg );
      void update_virtual_cpu_limit( const resource_limits_config_object& cfg );

      uint64_t pending_net_usage = 0ULL;
      uint64_t pending_cpu_usage = 0ULL;

      uint64_t total_net_weight = 0ULL;
      uint64_t total_cpu_weight = 0ULL;
      uint64_t total_ram_bytes = 0ULL;

      /**
       * The virtual number of bytes that would be consumed over blocksize_average_window_ms
       * if all blocks were at their maximum virtual size. This is virtual because the
       * real maximum block is less, this virtual number is only used for rate limiting users.
       *
       * It's lowest possible value is max_block_size * blocksize_average_window_ms / block_interval
       * It's highest possible value is 1000 times its lowest possible value
       *
       * This means that the most an account can consume during idle periods is 1000x the bandwidth
       * it is gauranteed under congestion.
       *
       * Increases when average_block_size < target_block_size, decreases when
       * average_block_size > target_block_size, with a cap at 1000x max_block_size
       * and a floor at max_block_size;
       **/
      uint64_t virtual_net_limit = 0ULL;

      /**
       *  Increases when average_bloc
       */
      uint64_t virtual_cpu_limit = 0ULL;

   };

   using resource_limits_state_index = chainbase::shared_multi_index_container<
      resource_limits_state_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<resource_limits_state_object, resource_limits_state_object::id_type, &resource_limits_state_object::id>>
      >
   >;

} } } /// eosio::chain::resource_limits

CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits::resource_limits_object,        eosio::chain::resource_limits::resource_limits_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits::resource_usage_object,         eosio::chain::resource_limits::resource_usage_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits::resource_limits_config_object, eosio::chain::resource_limits::resource_limits_config_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::resource_limits::resource_limits_state_object,  eosio::chain::resource_limits::resource_limits_state_index)
