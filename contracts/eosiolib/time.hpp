#pragma once
#include <stdint.h>
#include <string>
#include <eosiolib/eosio.hpp>

namespace eosio {
  class microseconds {
    public:
        explicit microseconds( int64_t c = 0) :_count(c){}
        static microseconds maximum() { return microseconds(0x7fffffffffffffffll); }
        friend microseconds operator + (const  microseconds& l, const microseconds& r ) { return microseconds(l._count+r._count); }
        friend microseconds operator - (const  microseconds& l, const microseconds& r ) { return microseconds(l._count-r._count); }


        bool operator==(const microseconds& c)const { return _count == c._count; }
        bool operator!=(const microseconds& c)const { return _count != c._count; }
        friend bool operator>(const microseconds& a, const microseconds& b){ return a._count > b._count; }
        friend bool operator>=(const microseconds& a, const microseconds& b){ return a._count >= b._count; }
        friend bool operator<(const microseconds& a, const microseconds& b){ return a._count < b._count; }
        friend bool operator<=(const microseconds& a, const microseconds& b){ return a._count <= b._count; }
        microseconds& operator+=(const microseconds& c) { _count += c._count; return *this; }
        microseconds& operator-=(const microseconds& c) { _count -= c._count; return *this; }
        int64_t count()const { return _count; }
        int64_t to_seconds()const { return _count/1000000; }

        int64_t _count;
        EOSLIB_SERIALIZE( microseconds, (_count) )
    private:
        friend class time_point;
  };

  inline microseconds seconds( int64_t s ) { return microseconds( s * 1000000 ); }
  inline microseconds milliseconds( int64_t s ) { return microseconds( s * 1000 ); }
  inline microseconds minutes(int64_t m) { return seconds(60*m); }
  inline microseconds hours(int64_t h) { return minutes(60*h); }
  inline microseconds days(int64_t d) { return hours(24*d); }

  class time_point {
    public:
        explicit time_point( microseconds e = microseconds() ) :elapsed(e){}
        operator std::string()const;
        static time_point from_iso_string( const std::string& s );
        const microseconds& time_since_epoch()const { return elapsed; }
        uint32_t            sec_since_epoch()const  { return uint32_t(elapsed.count() / 1000000); }
        bool   operator > ( const time_point& t )const                              { return elapsed._count > t.elapsed._count; }
        bool   operator >=( const time_point& t )const                              { return elapsed._count >=t.elapsed._count; }
        bool   operator < ( const time_point& t )const                              { return elapsed._count < t.elapsed._count; }
        bool   operator <=( const time_point& t )const                              { return elapsed._count <=t.elapsed._count; }
        bool   operator ==( const time_point& t )const                              { return elapsed._count ==t.elapsed._count; }
        bool   operator !=( const time_point& t )const                              { return elapsed._count !=t.elapsed._count; }
        time_point&  operator += ( const microseconds& m)                           { elapsed+=m; return *this;                 }
        time_point&  operator -= ( const microseconds& m)                           { elapsed-=m; return *this;                 }
        time_point   operator + (const microseconds& m) const { return time_point(elapsed+m); }
        time_point   operator + (const time_point& m) const { return time_point(elapsed+m.elapsed); }
        time_point   operator - (const microseconds& m) const { return time_point(elapsed-m); }
        microseconds operator - (const time_point& m) const { return microseconds(elapsed.count() - m.elapsed.count()); }
        microseconds elapsed;
        EOSLIB_SERIALIZE( time_point, (elapsed) )
  };

  /**
   *  A lower resolution time_point accurate only to seconds from 1970
   */
  class time_point_sec
  {
    public:
        time_point_sec()
        :utc_seconds(0){}

        explicit time_point_sec(uint32_t seconds )
        :utc_seconds(seconds){}

        time_point_sec( const time_point& t )
        :utc_seconds( uint32_t(t.time_since_epoch().count() / 1000000ll) ){}

        static time_point_sec maximum() { return time_point_sec(0xffffffff); }
        static time_point_sec min() { return time_point_sec(0); }

        operator time_point()const { return time_point( eosio::seconds( utc_seconds) ); }
        uint32_t sec_since_epoch()const { return utc_seconds; }

        time_point_sec operator = ( const eosio::time_point& t )
        {
          utc_seconds = uint32_t(t.time_since_epoch().count() / 1000000ll);
          return *this;
        }
        friend bool      operator < ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds < b.utc_seconds; }
        friend bool      operator > ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds > b.utc_seconds; }
        friend bool      operator <= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds <= b.utc_seconds; }
        friend bool      operator >= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds >= b.utc_seconds; }
        friend bool      operator == ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds == b.utc_seconds; }
        friend bool      operator != ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds != b.utc_seconds; }
        time_point_sec&  operator += ( uint32_t m ) { utc_seconds+=m; return *this; }
        time_point_sec&  operator += ( microseconds m ) { utc_seconds+=m.to_seconds(); return *this; }
        time_point_sec&  operator += ( time_point_sec m ) { utc_seconds+=m.utc_seconds; return *this; }
        time_point_sec&  operator -= ( uint32_t m ) { utc_seconds-=m; return *this; }
        time_point_sec&  operator -= ( microseconds m ) { utc_seconds-=m.to_seconds(); return *this; }
        time_point_sec&  operator -= ( time_point_sec m ) { utc_seconds-=m.utc_seconds; return *this; }
        time_point_sec   operator +( uint32_t offset )const { return time_point_sec(utc_seconds + offset); }
        time_point_sec   operator -( uint32_t offset )const { return time_point_sec(utc_seconds - offset); }

        friend time_point   operator + ( const time_point_sec& t, const microseconds& m )   { return time_point(t) + m;             }
        friend time_point   operator - ( const time_point_sec& t, const microseconds& m )   { return time_point(t) - m;             }
        friend microseconds operator - ( const time_point_sec& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        friend microseconds operator - ( const time_point& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        uint32_t utc_seconds;

        EOSLIB_SERIALIZE( time_point_sec, (utc_seconds) )
  };

   /**
   * This class is used in the block headers to represent the block time
   * It is a parameterised class that takes an Epoch in milliseconds and
   * and an interval in milliseconds and computes the number of slots.
   **/
   class block_timestamp {
      public:
         explicit block_timestamp( uint32_t s=0 ) :slot(s){}

         block_timestamp(const time_point& t) {
            set_time_point(t);
         }

         block_timestamp(const time_point_sec& t) {
            set_time_point(t);
         }

         static block_timestamp maximum() { return block_timestamp( 0xffff ); }
         static block_timestamp min() { return block_timestamp(0); }

         block_timestamp next() const {
            eosio_assert( std::numeric_limits<uint32_t>::max() - slot >= 1, "block timestamp overflow" );
            auto result = block_timestamp(*this);
            result.slot += 1;
            return result;
         }

         time_point to_time_point() const {
            return (time_point)(*this);
         }

         operator time_point() const {
            int64_t msec = slot * (int64_t)block_interval_ms;
            msec += block_timestamp_epoch;
            return time_point(milliseconds(msec));
         }

         void operator = (const time_point& t ) {
            set_time_point(t);
         }

         bool   operator > ( const block_timestamp& t )const   { return slot >  t.slot; }
         bool   operator >=( const block_timestamp& t )const   { return slot >= t.slot; }
         bool   operator < ( const block_timestamp& t )const   { return slot <  t.slot; }
         bool   operator <=( const block_timestamp& t )const   { return slot <= t.slot; }
         bool   operator ==( const block_timestamp& t )const   { return slot == t.slot; }
         bool   operator !=( const block_timestamp& t )const   { return slot != t.slot; }
         uint32_t slot;

         EOSLIB_SERIALIZE( block_timestamp, (slot) )
      private:
      static constexpr uint32_t block_interval_ms = 500;
      static constexpr uint64_t block_timestamp_epoch = 946684800000ll;  // epoch is year 2000

      void set_time_point(const time_point& t) {
         auto micro_since_epoch = t.time_since_epoch();
         auto msec_since_epoch  = micro_since_epoch.count() / 1000;
         slot = uint32_t(( msec_since_epoch - block_timestamp_epoch ) / block_interval_ms);
      }

      void set_time_point(const time_point_sec& t) {
         uint64_t  sec_since_epoch = t.sec_since_epoch();
         slot = uint32_t((sec_since_epoch * 1000 - block_timestamp_epoch) / block_interval_ms);
      }
   }; // block_timestamp
   typedef block_timestamp block_timestamp_type; 

} // namespace eosio
