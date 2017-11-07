#pragma once
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
  template<uint16_t Interval, uint64_t EpochMs>
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
      int64_t msec = slot * (int64_t)Interval;
      msec += EpochMs;
      return fc::time_point(fc::microseconds(msec * 1000));
   }

   void operator = (const fc::time_point& t ) {
      set_time_point(t);
   }

   void operator = (const fc::time_point_sec& t ) {
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
           slot = ( msec_since_epoch - EpochMs ) / Interval;
   }

   void set_time_point(const fc::time_point_sec& t) {
	   uint64_t  sec_since_epoch = t.sec_since_epoch();
	   slot = (sec_since_epoch * 1000 - EpochMs) / Interval;
   }
  }; // block_timestamp

  typedef block_timestamp<3000,946684800000ll> block_timestamp_type; // epoch is year 2000.

} } /// eosio::chain


#include <fc/reflect/reflect.hpp>
FC_REFLECT(eosio::chain::block_timestamp_type, (slot))

namespace fc {
  template<uint16_t Interval, uint64_t EpochMs>
  void to_variant(const eosio::chain::block_timestamp<Interval,EpochMs>& t, fc::variant& v) {
     auto tp = (fc::time_point)t;
     to_variant(tp, v);
  }

  template<uint16_t Interval, uint64_t EpochMs>
  void from_variant(const fc::variant& v, eosio::chain::block_timestamp<Interval,EpochMs>& t) {
     fc::microseconds mc;
     from_variant(v, mc);
     t = fc::time_point(mc); 
  }
}

#ifdef _MSC_VER
  #pragma warning (pop)
#endif /// #ifdef _MSC_VER
