#pragma once
#include <eosio/chain/config.hpp>

#include <stdint.h>
#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <fc/string.hpp>
#include <fc/optional.hpp>

namespace eosio { namespace chain {

   /**
   * This class is used in the block headers to represent the block time
   * It is a parameterised class that takes an Epoch in milliseconds and
   * and an interval in milliseconds and computes the number of slots.
   **/
   template<uint16_t IntervalMs, uint64_t EpochMs>
   class block_timestamp {
      public:
         explicit block_timestamp( uint32_t s=0 ) :slot(s){}

         block_timestamp(const fc::time_point& t) {
            set_time_point(t);
         }

         block_timestamp(const fc::time_point_sec& t) {
            set_time_point(t);
         }

         static block_timestamp maximum() { return block_timestamp( 0xffff ); }
         static block_timestamp min() { return block_timestamp(0); }

         operator fc::time_point() const {
            int64_t msec = slot * (int64_t)IntervalMs;
            msec += EpochMs;
            return fc::time_point(fc::milliseconds(msec));
         }

         void operator = (const fc::time_point& t ) {
            set_time_point(t);
         }

         bool   operator > ( const block_timestamp& t )const   { return slot >  t.slot; }
         bool   operator >=( const block_timestamp& t )const   { return slot >= t.slot; }
         bool   operator < ( const block_timestamp& t )const   { return slot <  t.slot; }
         bool   operator <=( const block_timestamp& t )const   { return slot <= t.slot; }
         bool   operator ==( const block_timestamp& t )const   { return slot == t.slot; }
         bool   operator !=( const block_timestamp& t )const   { return slot != t.slot; }
         uint32_t slot;

      private:
      void set_time_point(const fc::time_point& t) {
         auto micro_since_epoch = t.time_since_epoch();
         auto msec_since_epoch  = micro_since_epoch.count() / 1000;
         slot = ( msec_since_epoch - EpochMs ) / IntervalMs;
      }

      void set_time_point(const fc::time_point_sec& t) {
         uint64_t  sec_since_epoch = t.sec_since_epoch();
         slot = (sec_since_epoch * 1000 - EpochMs) / IntervalMs;
      }
   }; // block_timestamp

   typedef block_timestamp<config::block_interval_ms,config::block_timestamp_epoch> block_timestamp_type; 

} } /// eosio::chain


#include <fc/reflect/reflect.hpp>
FC_REFLECT(eosio::chain::block_timestamp_type, (slot))

namespace fc {
  template<uint16_t IntervalMs, uint64_t EpochMs>
  void to_variant(const eosio::chain::block_timestamp<IntervalMs,EpochMs>& t, fc::variant& v) {
     to_variant( (fc::time_point)t, v);
  }

  template<uint16_t IntervalMs, uint64_t EpochMs>
  void from_variant(const fc::variant& v, eosio::chain::block_timestamp<IntervalMs,EpochMs>& t) {
     t = v.as<fc::time_point>();
  }
}

#ifdef _MSC_VER
  #pragma warning (pop)
#endif /// #ifdef _MSC_VER
