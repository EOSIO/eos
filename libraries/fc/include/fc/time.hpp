#pragma once
#include <stdint.h>
#include <fc/string.hpp>
#include <fc/optional.hpp>

#ifdef _MSC_VER
  #pragma warning (push)
  #pragma warning (disable : 4244)
#endif //// _MSC_VER

namespace fc {
  class microseconds {
    public:
        constexpr explicit microseconds( int64_t c = 0) :_count(c){}
        static constexpr microseconds maximum() { return microseconds(0x7fffffffffffffffll); }
        friend constexpr microseconds operator + (const  microseconds& l, const microseconds& r ) { return microseconds(l._count+r._count); }
        friend constexpr microseconds operator - (const  microseconds& l, const microseconds& r ) { return microseconds(l._count-r._count); }

        constexpr bool operator==(const microseconds& c)const { return _count == c._count; }
        constexpr bool operator!=(const microseconds& c)const { return _count != c._count; }
        friend constexpr bool operator>(const microseconds& a, const microseconds& b){ return a._count > b._count; }
        friend constexpr bool operator>=(const microseconds& a, const microseconds& b){ return a._count >= b._count; }
        constexpr friend bool operator<(const microseconds& a, const microseconds& b){ return a._count < b._count; }
        constexpr friend bool operator<=(const microseconds& a, const microseconds& b){ return a._count <= b._count; }
        constexpr microseconds& operator+=(const microseconds& c) { _count += c._count; return *this; }
        constexpr microseconds& operator-=(const microseconds& c) { _count -= c._count; return *this; }
        constexpr int64_t count()const { return _count; }
        constexpr int64_t to_seconds()const { return _count/1000000; }
    private:
        friend class time_point;
        int64_t      _count;
  };
  inline constexpr microseconds seconds( int64_t s ) { return microseconds( s * 1000000 ); }
  inline constexpr microseconds milliseconds( int64_t s ) { return microseconds( s * 1000 ); }
  inline constexpr microseconds minutes(int64_t m) { return seconds(60*m); }
  inline constexpr microseconds hours(int64_t h) { return minutes(60*h); }
  inline constexpr microseconds days(int64_t d) { return hours(24*d); }

  class variant;
  void to_variant( const fc::microseconds&,  fc::variant&  );
  void from_variant( const fc::variant& , fc::microseconds& );

  class time_point {
    public:
        constexpr explicit time_point( microseconds e = microseconds() ) :elapsed(e){}
        static time_point now();
        static constexpr time_point maximum() { return time_point( microseconds::maximum() ); }
        static constexpr time_point min() { return time_point();                      }

        operator fc::string()const;
        static time_point from_iso_string( const fc::string& s );

        constexpr const microseconds& time_since_epoch()const { return elapsed; }
        constexpr uint32_t            sec_since_epoch()const  { return elapsed.count() / 1000000; }
        constexpr bool   operator > ( const time_point& t )const                              { return elapsed._count > t.elapsed._count; }
        constexpr bool   operator >=( const time_point& t )const                              { return elapsed._count >=t.elapsed._count; }
        constexpr bool   operator < ( const time_point& t )const                              { return elapsed._count < t.elapsed._count; }
        constexpr bool   operator <=( const time_point& t )const                              { return elapsed._count <=t.elapsed._count; }
        constexpr bool   operator ==( const time_point& t )const                              { return elapsed._count ==t.elapsed._count; }
        constexpr bool   operator !=( const time_point& t )const                              { return elapsed._count !=t.elapsed._count; }
        constexpr time_point&  operator += ( const microseconds& m)                           { elapsed+=m; return *this;                 }
        constexpr time_point&  operator -= ( const microseconds& m)                           { elapsed-=m; return *this;                 }
        constexpr time_point   operator + (const microseconds& m) const { return time_point(elapsed+m); }
        constexpr time_point   operator - (const microseconds& m) const { return time_point(elapsed-m); }
       constexpr microseconds operator - (const time_point& m) const { return microseconds(elapsed.count() - m.elapsed.count()); }
    private:
        microseconds elapsed;
  };

  /**
   *  A lower resolution time_point accurate only to seconds from 1970
   */
  class time_point_sec
  {
    public:
        constexpr time_point_sec()
        :utc_seconds(0){}

        constexpr explicit time_point_sec(uint32_t seconds )
        :utc_seconds(seconds){}

        constexpr time_point_sec( const time_point& t )
        :utc_seconds( t.time_since_epoch().count() / 1000000ll ){}

        static constexpr time_point_sec maximum() { return time_point_sec(0xffffffff); }
        static constexpr time_point_sec min() { return time_point_sec(0); }

        constexpr operator time_point()const { return time_point( fc::seconds( utc_seconds) ); }
        constexpr uint32_t sec_since_epoch()const { return utc_seconds; }

        constexpr time_point_sec operator = ( const fc::time_point& t )
        {
          utc_seconds = t.time_since_epoch().count() / 1000000ll;
          return *this;
        }
        constexpr friend bool      operator < ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds < b.utc_seconds; }
        constexpr friend bool      operator > ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds > b.utc_seconds; }
        constexpr friend bool      operator <= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds <= b.utc_seconds; }
        constexpr friend bool      operator >= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds >= b.utc_seconds; }
        constexpr friend bool      operator == ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds == b.utc_seconds; }
        constexpr friend bool      operator != ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds != b.utc_seconds; }
        constexpr time_point_sec&  operator += ( uint32_t m ) { utc_seconds+=m; return *this; }
        constexpr time_point_sec&  operator += ( microseconds m ) { utc_seconds+=m.to_seconds(); return *this; }
        constexpr time_point_sec&  operator -= ( uint32_t m ) { utc_seconds-=m; return *this; }
        constexpr time_point_sec&  operator -= ( microseconds m ) { utc_seconds-=m.to_seconds(); return *this; }
        constexpr time_point_sec   operator +( uint32_t offset )const { return time_point_sec(utc_seconds + offset); }
        constexpr time_point_sec   operator -( uint32_t offset )const { return time_point_sec(utc_seconds - offset); }

        friend constexpr time_point   operator + ( const time_point_sec& t, const microseconds& m )   { return time_point(t) + m;             }
        friend constexpr time_point   operator - ( const time_point_sec& t, const microseconds& m )   { return time_point(t) - m;             }
        friend constexpr microseconds operator - ( const time_point_sec& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        friend constexpr microseconds operator - ( const time_point& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }

        fc::string to_non_delimited_iso_string()const;
        fc::string to_iso_string()const;

        operator fc::string()const;
        static time_point_sec from_iso_string( const fc::string& s );

    private:
        uint32_t utc_seconds;
  };

  typedef fc::optional<time_point> otime_point;

  /** return a human-readable approximate time, relative to now()
   * e.g., "4 hours ago", "2 months ago", etc.
   */
  string get_approximate_relative_time_string(const time_point_sec& event_time,
                                              const time_point_sec& relative_to_time = fc::time_point::now(),
                                              const std::string& ago = " ago");
  string get_approximate_relative_time_string(const time_point& event_time,
                                              const time_point& relative_to_time = fc::time_point::now(),
                                              const std::string& ago = " ago");
}

#include <fc/reflect/reflect.hpp>
FC_REFLECT_TYPENAME( fc::time_point )
FC_REFLECT_TYPENAME( fc::microseconds )
FC_REFLECT_TYPENAME( fc::time_point_sec )

#ifdef _MSC_VER
  #pragma warning (pop)
#endif /// #ifdef _MSC_VER
