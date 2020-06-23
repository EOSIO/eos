#pragma once
#include <stdint.h>
#include <fc/string.hpp>
#include <fc/optional.hpp>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/convert.hpp>
#include <boost/convert/stream.hpp>

#ifdef _MSC_VER
  #pragma warning (push)
  #pragma warning (disable : 4244)
#endif //// _MSC_VER

#define TIME_IS_STD_CHRONO

namespace fc {
  namespace chrono = std::chrono;

  // fc::microseconds based on chrono has a different min() function.
  // Old microseconds::min() used to return 0
  // chrono::microseconds::min() returns numeric_limits<int64_t>::lowest()
  // Use microseconds::zero() to have the same functionality as before
  using     chrono::microseconds;  // uses int64_t
  using     chrono::milliseconds;  // uses int64_t
  // For compatibility with the old version of time_point_sec, uint32_t is used instead of int64_t.
  // UInt32 limits the date range to [1970-2106] and it should be fine for our purposes
  typedef   chrono::duration<uint32_t, std::ratio<1, 1>> seconds;
  using     chrono::hours;         // uses int32_t
  using     chrono::minutes;       // uses int32_t
  typedef   chrono::duration<int16_t, std::ratio<60 * 60 * 24>>      days;
  typedef   chrono::duration<int16_t, std::ratio<60 * 60 * 24 * 7>>  weeks;

  using     chrono::duration_cast;
  using     chrono::time_point_cast;

  typedef   chrono::time_point<chrono::system_clock, fc::microseconds>  time_point;
  typedef   chrono::time_point<chrono::system_clock, fc::seconds>       time_point_sec;

  template<typename Duration = fc::microseconds>
  auto now() { return fc::time_point_cast<Duration>( chrono::system_clock::now() ); }

  template <class Clock, class Duration>
  inline boost::posix_time::ptime  to_ptime(const chrono::time_point<Clock, Duration> &  tp) {
    boost::posix_time::microseconds usec(
      chrono::duration_cast<fc::microseconds>( tp.time_since_epoch() ).count() );
    return boost::posix_time::from_time_t(0) + usec;
  }

  std::string to_iso_string(const time_point &tp, bool extend_to_usec = false);
  std::string to_iso_string(const time_point_sec &tp);

  inline std::string to_non_delimited_iso_string(const time_point_sec &  tp);

  template <class Duration>
  inline std::string to_string(const Duration &  time) {
    #define TRY_TO_STRING(TYPE, TOKEN) \
      if (std::is_same<Duration, TYPE>::value) \
        return to_string( (int64_t)time.count() ) + TOKEN;

    TRY_TO_STRING( fc::microseconds, " microsec" );
    TRY_TO_STRING( fc::milliseconds, " millisec" );
    TRY_TO_STRING( fc::seconds,      " sec"      );
    TRY_TO_STRING( fc::minutes,      " min"      );
    TRY_TO_STRING( fc::hours,        " hours"    );
    TRY_TO_STRING( fc::days,         " days"     );
    TRY_TO_STRING( fc::weeks,        " weeks"    );
    return to_string( uint64_t(time.count()) );

    #undef TRY_TO_STRING
  }

  void from_iso_string( const fc::string &  s, fc::time_point_sec &  tp );
  void from_iso_string( const fc::string &  s, fc::time_point     &  tp );
  fc::microseconds from_string_to_usec( const fc::string &  str );
  template<class Dur>
  Dur from_string( const fc::string& str ) {
    return fc::duration_cast<Dur>( from_string_to_usec(str) );
  }

  template<class TimePoint>
  TimePoint from_iso_string( const fc::string& s )
  {
    TimePoint  res;
    from_iso_string(s, res);
    return res;
  }

  class variant;
  void from_variant( const fc::variant& v, fc::microseconds& t );
  void from_variant( const fc::variant& v, fc::time_point& tp );
  void from_variant( const fc::variant& v, fc::time_point_sec& tp );
  void to_variant( const fc::microseconds& t, variant& v );
  void to_variant( const fc::time_point& t, variant& v );
  void to_variant( const fc::time_point_sec& t, variant& v );

  /** return a human-readable approximate time, relative to now()
   * e.g., "4 hours ago", "2 months ago", etc.
   */
  string get_approximate_relative_time_string(const time_point_sec& event_time,
                                              const time_point_sec& relative_to_time = fc::now<fc::seconds>(),
                                              const std::string& ago = " ago");
  string get_approximate_relative_time_string(const time_point& event_time,
                                              const time_point& relative_to_time = fc::now(),
                                              const std::string& ago = " ago");
}

namespace std {
  template <class CharT, class Traits, class Rep, class Period>
  std::basic_ostream<CharT, Traits> &
  operator<<( std::basic_ostream<CharT, Traits>&        os,
              const fc::chrono::duration<Rep, Period>&  d ) {
    return os << fc::to_string(d);
  }

  template<class CharT, class Traits, class Clock, class Duration>
  std::basic_ostream<CharT, Traits> &
  operator<<( std::basic_ostream<CharT, Traits>&       os,
              const fc::chrono::time_point<Clock, Duration> &  tp ) {
    return os << fc::to_iso_string(tp, true);
  }

  template<class Clock, class Duration>
  struct less<fc::chrono::time_point<Clock, Duration>> {
    public:
      typedef fc::chrono::time_point<Clock, Duration> TP;
      constexpr bool operator()( const TP &  left, const TP &  right) const {
        return left.time_since_epoch().count() < right.time_since_epoch().count();
      }
  };

} // namespace std

#include <fc/reflect/reflect.hpp>
FC_REFLECT_TYPENAME( fc::microseconds )
FC_REFLECT_TYPENAME( fc::time_point )
FC_REFLECT_TYPENAME( fc::time_point_sec )

#ifdef _MSC_VER
  #pragma warning (pop)
#endif /// #ifdef _MSC_VER
