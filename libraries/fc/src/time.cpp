#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
#include <fc/string.hpp>
#include <fc/exception/exception.hpp>

#include <regex>

namespace fc {

  std::string to_iso_string(const time_point &tp, bool extend_to_usec /* = false*/)
  {
    auto count = tp.time_since_epoch().count();
    if (count >= 0)
    {
      time_t secs = (uint64_t)count / 1000000ULL;
      uint16_t msec = ((uint64_t)count % 1000000ULL) / 1000ULL;
      string padded_ms = to_string(1000ULL + msec);
      padded_ms[0] = '.';
      const auto ptime = boost::posix_time::from_time_t(secs);
      return boost::posix_time::to_iso_extended_string(ptime) + padded_ms;
    }
    else
    {
      // negative time_points serialized as "durations" in the ISO form with boost
      // this is not very human readable but fits the precedent set by the above
      auto as_duration = boost::posix_time::microseconds(count);
      return boost::posix_time::to_iso_string(as_duration);
    }
  }

  std::string to_iso_string(const time_point_sec &tp)
  {
    const auto ptime = boost::posix_time::from_time_t(time_t( tp.time_since_epoch().count() ));
    return boost::posix_time::to_iso_extended_string(ptime);
  }

  inline std::string to_non_delimited_iso_string(const time_point_sec &tp)
  {
    return boost::posix_time::to_iso_string(to_ptime(tp));
  }

  void from_iso_string( const fc::string& s, time_point_sec &  tp )
  { try {
      static boost::posix_time::ptime epoch = boost::posix_time::from_time_t( 0 );
      boost::posix_time::ptime pt;
      if( s.size() >= 5 && s.at( 4 ) == '-' ) // http://en.wikipedia.org/wiki/ISO_8601
          pt = boost::date_time::parse_delimited_time<boost::posix_time::ptime>( s, 'T' );
      else
          pt = boost::posix_time::from_iso_string( s );
      tp = fc::time_point_sec( fc::time_point_sec::duration( (pt - epoch).total_seconds() ) );
  } FC_RETHROW_EXCEPTIONS(warn, "unable to convert the '${str}' string to fc::time_point_sec. Make sure the string is ISO-formatted", ("str", s)) }

  void from_iso_string( const fc::string& s, time_point &  tp )
  { try {
      auto tps = from_iso_string<time_point_sec>(s);
      tp = fc::time_point(tps);

      auto dot = s.find( '.' );
      if( dot != std::string::npos ) {
         auto ms = s.substr( dot );
         ms[0] = '1';
         ms.resize(7, '0');
         tp += fc::microseconds( to_int64(ms) - 1000000 );
      }
  } FC_RETHROW_EXCEPTIONS(warn, "unable to convert the '${str}' string to fc::time_point. Make sure the string is ISO-formatted", ("str", s)) }

  fc::microseconds from_string_to_usec( const fc::string& str )
  { try {
    if (str == "-1") {
      return fc::microseconds::max();
    }

    static std::unordered_set<std::string> microsec = { "us", "usec", "microsec", "microsecond", "microseconds" };
    static std::unordered_set<std::string> millisec = { "ms", "msec", "millisec", "millisecond", "milliseconds" };
    static std::unordered_set<std::string> sec      = { "s", "sec", "second", "seconds" };
    static std::unordered_set<std::string> min      = { "min", "minute", "minutes" };
    static std::unordered_set<std::string> hour     = { "h", "hour", "hours" };
    static std::unordered_set<std::string> day      = { "d", "day", "days" };
    static std::unordered_set<std::string> week     = { "w", "week", "weeks" };

    static std::regex r{std::string("^\\s*([0-9]+)\\s*(") +
                        boost::algorithm::join(microsec, "|") + '|' +
                        boost::algorithm::join(millisec, "|") + '|' +
                        boost::algorithm::join(sec,      "|") + '|' +
                        boost::algorithm::join(min,      "|") + '|' +
                        boost::algorithm::join(hour,     "|") + '|' +
                        boost::algorithm::join(day,      "|") + '|' +
                        boost::algorithm::join(week,     "|") +
                        ")\\s*$"};

    std::smatch m;
    std::regex_search(str, m, r);

    FC_ASSERT(!m.empty(), "string is not a positive number with an optional suffix OR -1");
    int64_t number = std::stoll(m[1]);
    int64_t mult = 1'000'000;

    if (m.size() == 3 && m[2].length() != 0) {
      #define CHECK_SUFFIX(SUFFIX, SET)  SET.find(SUFFIX) != SET.end()

      auto suffix = m[2].str();
      if      (CHECK_SUFFIX(suffix, microsec)) { mult = 1; }
      else if (CHECK_SUFFIX(suffix, millisec)) { mult = 1000; }
      else if (CHECK_SUFFIX(suffix, sec))      { ; }
      else if (CHECK_SUFFIX(suffix, min))      { mult *= 60; }
      else if (CHECK_SUFFIX(suffix, hour))     { mult *= 60 * 60; }
      else if (CHECK_SUFFIX(suffix, day))      { mult *= 60 * 60 * 24; }
      else if (CHECK_SUFFIX(suffix, week))     { mult *= 60 * 60 * 24 * 7; }

      #undef CHECK_SUFFIX(SUFFIX, SET)
    }
    auto final = number * mult;
    FC_ASSERT(final / mult == number, "specified time overflows, use \"-1\" to represent infinite time");

    return fc::microseconds(final);
  } FC_RETHROW_EXCEPTIONS( warn, "unable to convert string to fc::duration<...>" ) }

  void to_variant( const fc::microseconds& t, variant& v ) {
    v = t.count();
  }
  void from_variant( const fc::variant& v, fc::microseconds& t ) {
    t = fc::microseconds( v.as_int64() );
  }
  void to_variant( const fc::time_point& t, variant& v ) {
    v = to_iso_string( t );
  }
  void from_variant( const fc::variant& v, fc::time_point& tp ) {
    fc::from_iso_string( v.as_string(), tp );
  }
  void to_variant( const fc::time_point_sec& t, variant& v ) {
    v = to_iso_string( t );
  }
  void from_variant( const fc::variant& v, fc::time_point_sec& tp ) {
    fc::from_iso_string( v.as_string(), tp );
  }

  // inspired by show_date_relative() in git's date.c
  string get_approximate_relative_time_string(const time_point_sec& event_time,
                                              const time_point_sec& relative_to_time /* = fc::now<fc::microseconds>() */,
                                              const std::string& default_ago /* = " ago" */) {
    string ago = default_ago;

    uint32_t r = fc::duration_cast<fc::seconds>( relative_to_time.time_since_epoch()).count();
    uint32_t e = fc::duration_cast<fc::seconds>( event_time.time_since_epoch()).count();

    int32_t seconds_ago;
    if (r < e) {
       ago = " in the future";
       seconds_ago = e - r;
    } else {
       seconds_ago = r - e;
    }

    std::stringstream result;
    if (seconds_ago < 90)
    {
      result << seconds_ago << " second" << (seconds_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t minutes_ago = (seconds_ago + 30) / 60;
    if (minutes_ago < 90)
    {
      result << minutes_ago << " minute" << (minutes_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t hours_ago = (minutes_ago + 30) / 60;
    if (hours_ago < 90)
    {
      result << hours_ago << " hour" << (hours_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t days_ago = (hours_ago + 12) / 24;
    if (days_ago < 90)
    {
      result << days_ago << " day" << (days_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t weeks_ago = (days_ago + 3) / 7;
    if (weeks_ago < 70)
    {
      result << weeks_ago << " week" << (weeks_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t months_ago = (days_ago + 15) / 30;
    if (months_ago < 12)
    {
      result << months_ago << " month" << (months_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t years_ago = days_ago / 365;
    result << years_ago << " year" << (months_ago > 1 ? "s" : "");
    if (months_ago < 12 * 5)
    {
      uint32_t leftover_days = days_ago - (years_ago * 365);
      uint32_t leftover_months = (leftover_days + 15) / 30;
      if (leftover_months)
        result << leftover_months <<  " month" << (months_ago > 1 ? "s" : "");
    }
    result << ago;
    return result.str();
  }

  string get_approximate_relative_time_string(const time_point& event_time,
                                              const time_point& relative_to_time /* = fc::now() */,
                                              const std::string& ago /* = " ago" */) {
    return get_approximate_relative_time_string(
      fc::time_point_cast<fc::seconds>(event_time),
      fc::time_point_cast<fc::seconds>(relative_to_time),
      ago );
    }

} //namespace fc
