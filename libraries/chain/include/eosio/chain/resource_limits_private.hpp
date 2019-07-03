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

         void add(int64_t units_signed, uint32_t ordinal, uint32_t window_size)
         {
            uint64_t units = std::abs(units_signed);
            bool neg = units_signed < 0;
            // check for some numerical limits before doing any state mutations
            EOS_ASSERT(neg || units <= max_raw_value, rate_limiting_state_inconsistent, "Usage exceeds maximum value representable after extending for precision");
            EOS_ASSERT(neg || std::numeric_limits<decltype(consumed)>::max() - consumed >= units, rate_limiting_state_inconsistent, "Overflow in tracked usage when adding usage!");

            auto value_ex_contrib = downgrade_cast<uint64_t>(integer_divide_ceil((uint128_t)units * Precision, (uint128_t)window_size));
            EOS_ASSERT(neg || (std::numeric_limits<decltype(value_ex)>::max() - value_ex >= value_ex_contrib), rate_limiting_state_inconsistent, "Overflow in accumulated value when adding usage!");

            if(ordinal && (last_ordinal != ordinal) ) {
               EOS_ASSERT( ordinal > last_ordinal, resource_limit_exception, "new ordinal(${n}) cannot be less than the previous ordinal(${p})",("n", ordinal)("p", last_ordinal) );
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
            if (neg) {
               consumed -= std::min(consumed, units);
               value_ex -= std::min(value_ex, value_ex_contrib);
            }
            else {
               consumed += units;
               value_ex += value_ex_contrib;
            }
         }
      };
   }

   using usage_accumulator = impl::exponential_moving_average_accumulator<>;

   struct by_owner;
   struct by_dirty;

   struct resource_usage_object {
      CHAINDB_OBJECT_CTOR(resource_usage_object, owner.value)

      account_name owner;
       
      std::vector<usage_accumulator> accumulators;
   };

   using resource_usage_table = cyberway::chaindb::table_container<
      resource_usage_object,
       cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(resource_usage_object, account_name, owner) >
      >
   >;

   class resource_limits_config_object : public cyberway::chaindb::object<resource_limits_config_object_type, resource_limits_config_object> {
      CHAINDB_OBJECT_ID_CTOR(resource_limits_config_object);
      id_type id;
      std::vector<elastic_limit_parameters> limit_parameters;
      std::vector<uint32_t> account_usage_average_windows;
   };

   using resource_limits_config_table = cyberway::chaindb::table_container<
      resource_limits_config_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(resource_limits_config_object, resource_limits_config_object::id_type, id)>
      >
   >;

   class resource_limits_state_object : public cyberway::chaindb::object<resource_limits_state_object_type, resource_limits_state_object> {
      CHAINDB_OBJECT_ID_CTOR(resource_limits_state_object);
      id_type id;
      std::vector<usage_accumulator> block_usage_accumulators;
      std::vector<int64_t> pending_usage;
      std::vector<uint64_t> virtual_limits;
      void update_virtual_limit(const resource_limits_config_object& cfg, resource_id res);
      void add_pending_delta(int64_t delta, const chain_config& chain_cfg, resource_id res);
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

FC_REFLECT(eosio::chain::resource_limits::resource_usage_object, (owner)(accumulators))

FC_REFLECT(eosio::chain::resource_limits::resource_limits_config_object, (id)(limit_parameters)(account_usage_average_windows))

FC_REFLECT(eosio::chain::resource_limits::resource_limits_state_object, (id)(block_usage_accumulators)(pending_usage)(virtual_limits))
