#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/multi_index_includes.hpp>


namespace eosio { namespace chain { namespace resource_limits {
using namespace int_arithmetic;

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
               EOS_ASSERT( ordinal > last_ordinal, resource_limit_exception, "new ordinal cannot be less than the previous ordinal" );
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

   struct by_owner;
   struct by_dirty;

   struct resource_usage_object : public chainbase::object<resource_usage_object_type, resource_usage_object> {
      OBJECT_CTOR(resource_usage_object)

      id_type id;
      account_name owner;

      usage_accumulator        net_usage;
      usage_accumulator        cpu_usage;

      uint64_t                 ram_usage = 0;
      uint64_t                 ram_owned = 0;
      uint64_t                 storage_usage = 0;
      uint64_t                 storage_owned = 0;
   };

   using resource_usage_table = cyberway::chaindb::table_container<
      resource_usage_object,
       cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(resource_usage_object, resource_usage_object::id_type, id)>,
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_owner>, BOOST_MULTI_INDEX_MEMBER(resource_usage_object, account_name, owner) >
      >
   >;

   class resource_limits_config_object : public cyberway::chaindb::object<resource_limits_config_object_type, resource_limits_config_object> {
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

   using resource_limits_config_table = cyberway::chaindb::table_container<
      resource_limits_config_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(resource_limits_config_object, resource_limits_config_object::id_type, id)>
      >
   >;

   class resource_limits_state_object : public cyberway::chaindb::object<resource_limits_state_object_type, resource_limits_state_object> {
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
      
      uint64_t ram_usage = 0ULL;
      uint64_t storage_usage = 0ULL;

      void update_virtual_net_limit( const resource_limits_config_object& cfg );
      void update_virtual_cpu_limit( const resource_limits_config_object& cfg );

      uint64_t pending_net_usage = 0ULL;
      uint64_t pending_cpu_usage = 0ULL;

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
      
      //TODO: smoothly increase this value when starting the chain
      uint64_t virtual_ram_limit = config::default_virtual_ram_limit;
   };

   using resource_limits_state_table = cyberway::chaindb::table_container<
      resource_limits_state_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(resource_limits_state_object, resource_limits_state_object::id_type, id)>
      >
   >;

} } } /// eosio::chain::resource_limits

CHAINDB_SET_TABLE_TYPE(eosio::chain::resource_limits::resource_usage_object,         eosio::chain::resource_limits::resource_usage_table)
CHAINDB_SET_TABLE_TYPE(eosio::chain::resource_limits::resource_limits_config_object, eosio::chain::resource_limits::resource_limits_config_table)
CHAINDB_SET_TABLE_TYPE(eosio::chain::resource_limits::resource_limits_state_object,  eosio::chain::resource_limits::resource_limits_state_table)

CHAINDB_TAG(eosio::chain::resource_limits::resource_usage_object,         resusage)
CHAINDB_TAG(eosio::chain::resource_limits::resource_limits_config_object, resconfig)
CHAINDB_TAG(eosio::chain::resource_limits::resource_limits_state_object,  resstate)

FC_REFLECT(eosio::chain::resource_limits::usage_accumulator, (last_ordinal)(value_ex)(consumed))

FC_REFLECT(eosio::chain::resource_limits::resource_usage_object, (id)(owner)(net_usage)(cpu_usage)
    (ram_usage)(ram_owned)(storage_usage)(storage_owned))

FC_REFLECT(eosio::chain::resource_limits::resource_limits_config_object, (id)
    (cpu_limit_parameters)(net_limit_parameters)
    (account_cpu_usage_average_window)(account_net_usage_average_window))

FC_REFLECT(eosio::chain::resource_limits::resource_limits_state_object, (id)
    (average_block_net_usage)(average_block_cpu_usage)(ram_usage)(storage_usage)
    (pending_net_usage)(pending_cpu_usage)
    (virtual_net_limit)(virtual_cpu_limit)(virtual_ram_limit))