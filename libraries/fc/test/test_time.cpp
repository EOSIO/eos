/**
 *  @file
 *  @copyright defined in eosio.cdt/LICENSE.txt
 */
#define BOOST_TEST_MODULE fc_time
#include <boost/test/included/unit_test.hpp>

#include <fc/time.hpp>
#include <fc/exception/exception.hpp>

using namespace fc;
using namespace std::literals;
using std::numeric_limits;

using fc::microseconds;
using fc::seconds;
using fc::time_point;
using fc::time_point_sec;

static constexpr int64_t i32min = numeric_limits<int32_t>::min();
static constexpr int64_t i32max = numeric_limits<int32_t>::max();
static constexpr int64_t i64min = numeric_limits<int64_t>::min(); // -9223372036854775808
static constexpr int64_t i64max = numeric_limits<int64_t>::max(); //  9223372036854775807

const static std::set<std::string> microseconds_suffix = { "us", "usec", "microsec", "microsecond", "microseconds" };
const static std::set<std::string> milliseconds_suffix = { "ms", "msec", "millisec", "millisecond", "milliseconds" };
const static std::set<std::string> seconds_suffix      = { "s", "sec", "second", "seconds" };
const static std::set<std::string> minutes_suffix      = { "m", "min", "minute", "minutes" };
const static std::set<std::string> hours_suffix        = { "h", "hour", "hours" };
const static std::set<std::string> days_suffix         = { "d", "day", "days" };
const static std::set<std::string> weeks_suffix        = { "w", "week", "weeks" };

#ifdef TIME_IS_STD_CHRONO
   #define sec_since_epoch time_since_epoch
#endif

BOOST_AUTO_TEST_SUITE(fc_time)

// Definitions in `eosio.cdt/libraries/eosio/time.hpp`
BOOST_AUTO_TEST_CASE(microseconds_type_test) try {
   //// explicit microseconds(uint64_t)/int64_t count()
   BOOST_CHECK_EQUAL( microseconds{}.count(), 0ULL );
   BOOST_CHECK_EQUAL( microseconds{i64max}.count(), i64max );
   BOOST_CHECK_EQUAL( microseconds{i64min}.count(), i64min );

   // -----------------------------
   // static microseconds maximum()
   #ifdef TIME_IS_STD_CHRONO
      BOOST_CHECK_EQUAL( microseconds::max().count(),  i64max );
      BOOST_CHECK_EQUAL( microseconds::min().count(),  i64min );
      BOOST_CHECK_EQUAL( microseconds::zero().count(), 0 );
   #else
      BOOST_CHECK_EQUAL( microseconds::maximum(), microseconds{0x7FFFFFFFFFFFFFFFLL}.count() );
   #endif

   // ------------------------------------------------------------------------
   // friend microseconds operator+(const  microseconds&, const microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{0LL} + microseconds{ 1LL}), microseconds{ 1LL} );
   BOOST_CHECK_EQUAL( (microseconds{1LL} + microseconds{-1LL}), microseconds{ 0LL} );

   // ------------------------------------------------------------------------
   // friend microseconds operator-(const  microseconds&, const microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{0LL} - microseconds{ 1LL}), microseconds{-1LL} );
   BOOST_CHECK_EQUAL( (microseconds{1LL} - microseconds{-1LL}), microseconds{ 2LL} );

   // ----------------------------------------------
   // microseconds& operator+=(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{0LL} += microseconds{ 1LL}), microseconds{ 1LL} );
   BOOST_CHECK_EQUAL( (microseconds{1LL} += microseconds{-1LL}), microseconds{ 0LL} );

   // ----------------------------------------------
   // microseconds& operator-=(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{0LL} -= microseconds{ 1LL}), microseconds{-1LL} );
   BOOST_CHECK_EQUAL( (microseconds{1LL} -= microseconds{-1LL}), microseconds{ 2LL} );

   // -------------------------------------
   // bool operator==(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{1LL} == microseconds{1LL}), true  );
   BOOST_CHECK_EQUAL( (microseconds{0LL} == microseconds{1LL}), false );

   // -------------------------------------
   // bool operator!=(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{1LL} != microseconds{1LL}), false );
   BOOST_CHECK_EQUAL( (microseconds{0LL} != microseconds{1LL}), true  );

   // -------------------------------------
   // bool operator<(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{0LL} <  microseconds{1LL}), true  );
   BOOST_CHECK_EQUAL( (microseconds{1LL} <  microseconds{1LL}), false );

   // -------------------------------------
   // bool operator<=(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{1LL} <= microseconds{1LL}), true  );
   BOOST_CHECK_EQUAL( (microseconds{2LL} <= microseconds{1LL}), false );

   // -------------------------------------
   // bool operator>(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{1LL} >  microseconds{0LL}), true  );
   BOOST_CHECK_EQUAL( (microseconds{1LL} >  microseconds{1LL}), false );

   // -------------------------------------
   // bool operator>=(const  microseconds&)
   BOOST_CHECK_EQUAL( (microseconds{1LL} >= microseconds{1LL}), true  );
   BOOST_CHECK_EQUAL( (microseconds{1LL} >= microseconds{2LL}), false );

   // --------------------
   // int64_t to_seconds()
   #ifdef TIME_IS_STD_CHRONO
      int64_t max = fc::seconds::max().count() * 1000000;
      BOOST_CHECK_EQUAL( (fc::duration_cast<fc::seconds>(microseconds{max})).count(), max / 1000000 );
   #else
      BOOST_CHECK_EQUAL( (microseconds{i64max}.to_seconds()), i64max / 1000000 );
      // -----------------------------------------
      // inline microseconds milliseconds(int64_t)
      BOOST_CHECK_EQUAL( milliseconds(0LL),  microseconds{0LL} );
      BOOST_CHECK_EQUAL( milliseconds(1LL),  microseconds{1000LL} );
      BOOST_CHECK_EQUAL( milliseconds(60LL), microseconds{60LL*1000LL} );

      // ------------------------------------
      // inline microseconds seconds(int64_t)
      BOOST_CHECK_EQUAL( seconds(0LL),  microseconds{0LL} );
      BOOST_CHECK_EQUAL( seconds(1LL),  microseconds{1000000LL} );
      BOOST_CHECK_EQUAL( seconds(60LL), microseconds{60LL*1000000LL} );

      // ------------------------------------
      // inline microseconds minutes(int64_t)
      BOOST_CHECK_EQUAL( minutes(0LL),  microseconds{0LL} );
      BOOST_CHECK_EQUAL( minutes(1LL),  microseconds{60LL*1000000LL} );
      BOOST_CHECK_EQUAL( minutes(60LL), microseconds{60LL*60LL*1000000LL} );

      // ----------------------------------
      // inline microseconds hours(int64_t)
      BOOST_CHECK_EQUAL( hours(0LL),  microseconds{0LL} );
      BOOST_CHECK_EQUAL( hours(1LL),  microseconds{60LL*60LL*1000000LL} );
      BOOST_CHECK_EQUAL( hours(60LL), microseconds{60LL*60LL*60LL*1000000LL} );

      // ---------------------------------
      // inline microseconds days(int64_t)
      BOOST_CHECK_EQUAL( days(0LL),  microseconds{0LL} );
      BOOST_CHECK_EQUAL( days(1LL),  microseconds{24LL*60LL*60LL*1000000LL} );
      BOOST_CHECK_EQUAL( days(60LL), microseconds{24LL*60LL*60LL*60LL*1000000LL} );
   #endif
} FC_LOG_AND_RETHROW();

// Definitions in `eosio.cdt/libraries/eosio/time.hpp`
BOOST_AUTO_TEST_CASE(time_point_type_test) try {
   static const microseconds ms0 { 0LL};
   static const microseconds ms1 { 1LL};
   static const microseconds msn1{-1LL};
   static const microseconds ms_min{i64min};
   static const microseconds ms_max{i64max};

   //// explicit time_point(microseconds)
   // microseconds& time_since_epoch()
   BOOST_CHECK_EQUAL( time_point{ms0}.sec_since_epoch(), ms0 );
   BOOST_CHECK_EQUAL( time_point{ms1}.sec_since_epoch(), ms1 );
   BOOST_CHECK_EQUAL( time_point{ms_min}.sec_since_epoch(), ms_min );
   BOOST_CHECK_EQUAL( time_point{ms_max}.sec_since_epoch(), ms_max );

   // --------------------------
   // uint32_t sec_since_epoch()
   BOOST_CHECK_EQUAL( fc::duration_cast<fc::seconds>(time_point{ms0}.sec_since_epoch()).count(), 0 );
   BOOST_CHECK_EQUAL( fc::duration_cast<fc::seconds>(time_point{ms1}.sec_since_epoch()).count(), 0 );
   BOOST_CHECK_EQUAL( fc::duration_cast<fc::seconds>(time_point{microseconds{1000000}}.sec_since_epoch()).count(),   1 );
   BOOST_CHECK_EQUAL( fc::duration_cast<fc::seconds>(time_point{microseconds{10000000}}.sec_since_epoch()).count(), 10 );
   // -----------------------------------------
   // time_point operator+(const microseconds&)
   BOOST_CHECK_EQUAL( (time_point{ms0}  + ms1), time_point{ms1} );
   BOOST_CHECK_EQUAL( (time_point{msn1} + ms1), time_point{ms0} );

   // -----------------------------------------
   // time_point operator-(const microseconds&)
   BOOST_CHECK_EQUAL( (time_point{ms0} - ms1), time_point{msn1} );
   BOOST_CHECK_EQUAL( (time_point{ms0} - msn1), time_point{ms1} );

   // -------------------------------------------
   // time_point& operator+=(const microseconds&)
   BOOST_CHECK_EQUAL( (time_point{ms0} += ms1), time_point{ms1}  );
   BOOST_CHECK_EQUAL( (time_point{msn1} += ms1), time_point{ms0} );

   // -------------------------------------------
   // time_point& operator-=(const microseconds&)
   BOOST_CHECK_EQUAL( (time_point{ms0} -= ms1), time_point{msn1} );
   BOOST_CHECK_EQUAL( (time_point{ms0} -= msn1), time_point{ms1} );

   // ----------------------------------
   // bool operator==(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms0} == time_point{ms0}), true  );
   BOOST_CHECK_EQUAL( (time_point{ms0} == time_point{ms1}), false );

   // ----------------------------------
   // bool operator!=(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms0} != time_point{ms0}), false );
   BOOST_CHECK_EQUAL( (time_point{ms0} != time_point{ms1}), true  );

   // ---------------------------------
   // bool operator<(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms0} <  time_point{ms1}), true  );
   BOOST_CHECK_EQUAL( (time_point{ms1} <  time_point{ms1}), false );

   // ----------------------------------
   // bool operator<=(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms1} <= time_point{ms1}), true  );
   BOOST_CHECK_EQUAL( (time_point{ms1} <= time_point{ms0}), false );

   // ---------------------------------
   // bool operator>(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms1} >  time_point{ms0}), true  );
   BOOST_CHECK_EQUAL( (time_point{ms1} >  time_point{ms1}), false );

   // ----------------------------------
   // bool operator>=(const time_point&)
   BOOST_CHECK_EQUAL( (time_point{ms1} >= time_point{ms1}), true  );
   BOOST_CHECK_EQUAL( (time_point{ms0} >= time_point{ms1}), false );
} FC_LOG_AND_RETHROW();
/*
BOOST_AUTO_TEST_CASE(time_point_sec_type_test) try {
   static const microseconds ms0 { 0LL};
   static const microseconds ms1 { 1LL};
   static const microseconds msn1{-1LL};
   static const microseconds ms_min{i64min};
   static const microseconds ms_max{i64max};

   static const time_point tp0{ms0};
   static const time_point tp1{ms1};
   static const time_point tpn1{msn1};
   static const time_point tp_min{ms_min};
   static const time_point tp_max{ms_max};

   //// time_point_sec()
   //uint32_t sec_since_epoch()const
   BOOST_CHECK_EQUAL( time_point_sec{}.sec_since_epoch(), 0sec );

   //// explicit time_point_sec(uint32_t)
   #ifdef TIME_IS_STD_CHRONO
     BOOST_CHECK_EQUAL( time_point_sec{fc::seconds{u32min}}.sec_since_epoch().count(), 0 );
     BOOST_CHECK_EQUAL( time_point_sec{fc::seconds{u32max}}.sec_since_epoch().count(), u32max );
     BOOST_CHECK_EQUAL( time_point_sec{fc::seconds{u32max + 1}}.sec_since_epoch().count(), 0 );
   #else
     BOOST_CHECK_EQUAL( time_point_sec{u32min}.sec_since_epoch(), 0 );
     BOOST_CHECK_EQUAL( time_point_sec{u32max}.sec_since_epoch(), u32max );
     BOOST_CHECK_EQUAL( time_point_sec{u32max + 1}.sec_since_epoch(), 0 );
   #endif

   //// time_point_sec(const time_point&)
   #ifdef TIME_IS_STD_CHRONO
      BOOST_CHECK_EQUAL( time_point_sec{fc::time_point_cast<fc::seconds>(tp0)}.sec_since_epoch(), ms0.count() / 1000000 );
      BOOST_CHECK_EQUAL( time_point_sec{fc::time_point_cast<fc::seconds>(tp1)}.sec_since_epoch(), ms1.count() / 1000000 );
      BOOST_CHECK_EQUAL( time_point_sec{fc::time_point_cast<fc::seconds>(tpn1)}.sec_since_epoch(), msn1.count() / 1000000 );
   #else
      BOOST_CHECK_EQUAL( time_point_sec{tp0}.sec_since_epoch(), ms0.count() / 1000000 );
      BOOST_CHECK_EQUAL( time_point_sec{tp1}.sec_since_epoch(), ms1.count() / 1000000 );
      BOOST_CHECK_EQUAL( time_point_sec{tpn1}.sec_since_epoch(), msn1.count() / 1000000 );
   #endif

   // ---------------------------
   // static time_point_sec min()
   #ifdef TIME_IS_STD_CHRONO
      BOOST_CHECK_EQUAL( time_point_sec{}.min().sec_since_epoch().count() == 0, true );
      BOOST_CHECK_EQUAL( time_point_sec{}.min().sec_since_epoch().count() != 1, true );
   #else
      BOOST_CHECK_EQUAL( time_point_sec{}.minimum().sec_since_epoch() == 0, true );
      BOOST_CHECK_EQUAL( time_point_sec{}.minimum().sec_since_epoch() != 1, true );
   #endif

   // -------------------------------
   // static time_point_sec maximum()
   #ifdef TIME_IS_STD_CHRONO
      BOOST_CHECK_EQUAL( time_point_sec{}.max().sec_since_epoch().count() == 0xFFFFFFFF, true );
      BOOST_CHECK_EQUAL( time_point_sec{}.max().sec_since_epoch().count() != 1, true );
   #else
      BOOST_CHECK_EQUAL( time_point_sec{}.maximum().sec_since_epoch() == 0xFFFFFFFF, true );
      BOOST_CHECK_EQUAL( time_point_sec{}.maximum().sec_since_epoch() != 1, true );
   #endif

   // --------------------------
   // operator time_point()const
   BOOST_CHECK_EQUAL( time_point_sec{u32min}.operator time_point(), time_point{microseconds{static_cast<int64_t>(u32min)*1000000}} );
   BOOST_CHECK_EQUAL( time_point_sec{u32max}.operator time_point(), time_point{microseconds{static_cast<int64_t>(u32max)*1000000}} );

   // -------------------------------------------
   // time_point_sec operator=(const time_point&)
   BOOST_CHECK_EQUAL( (time_point_sec{} = tp0), time_point_sec{} );
   BOOST_CHECK_EQUAL( (time_point_sec{} = tp1), time_point_sec{} );
   BOOST_CHECK_EQUAL( (time_point_sec{} = tp_max), time_point_sec{tp_max} );
   // ---------------------------------------
   // time_point_sec operator+(uint32_t)const
   BOOST_CHECK_EQUAL( (time_point_sec{} + u32min), time_point_sec{u32min} );
   BOOST_CHECK_EQUAL( (time_point_sec{} + u32max), time_point_sec{u32max} );

   // -----------------------------------------------------------------------
   // friend time_point operator+(const time_point_sec&, const microseconds&)
   BOOST_CHECK_EQUAL( (time_point_sec{0} + microseconds{ 1000000LL}), time_point{microseconds{ 1000000LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} + microseconds{-1000000LL}), time_point{microseconds{-1000000LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} + microseconds{ 1000000LL}), time_point{microseconds{ 2000000LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} + microseconds{-1000000LL}), time_point{microseconds{       0LL}} );

   // -----------------------------------------------------------------------
   // friend time_point operator-(const time_point_sec&, const microseconds&)
   BOOST_CHECK_EQUAL( (time_point_sec{0} - microseconds{ 1000000LL}), time_point{microseconds{-1000000LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} - microseconds{-1000000LL}), time_point{microseconds{ 1000000LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} - microseconds{ 1000000LL}), time_point{microseconds{       0LL}} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} - microseconds{-1000000LL}), time_point{microseconds{ 2000000LL}} );

   // ---------------------------------------------------------------------------
   // friend microseconds operator-(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{0} - time_point_sec{0}), microseconds{       0LL} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} - time_point_sec{1}), microseconds{-1000000LL} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} - time_point_sec{0}), microseconds{ 1000000LL} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} - time_point_sec{1}), microseconds{       0LL} );

   // -----------------------------------------------------------------------
   // friend microseconds operator-(const time_point&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point{microseconds{0}} - time_point_sec{0}), microseconds{       0LL} );
   BOOST_CHECK_EQUAL( (time_point{microseconds{0}} - time_point_sec{1}), microseconds{-1000000LL} );
   BOOST_CHECK_EQUAL( (time_point{microseconds{1}} - time_point_sec{0}), microseconds{       1LL} );
   BOOST_CHECK_EQUAL( (time_point{microseconds{1}} - time_point_sec{1}), microseconds{ -999999LL} );

   // ------------------------------------
   // time_point_sec& operator+=(uint32_t)
   BOOST_CHECK_EQUAL( (time_point_sec{0} += u32min), time_point_sec{u32min} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} += u32max), time_point_sec{u32max} );

   // ----------------------------------------
   // time_point_sec& operator+=(microseconds)
   BOOST_CHECK_EQUAL( (time_point_sec{0} += microseconds{      1LL}), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} += microseconds{1000000LL}), time_point_sec{1} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} += microseconds{      1LL}), time_point_sec{1} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} += microseconds{1000000LL}), time_point_sec{2} );

   // ------------------------------------
   // time_point_sec& operator-=(uint32_t)
   BOOST_CHECK_EQUAL( (time_point_sec{u32min} -= u32min), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{u32max} -= u32max), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{u32max} -= u32min), time_point_sec{u32max} );

   // ----------------------------------------
   // time_point_sec& operator-=(microseconds)
   BOOST_CHECK_EQUAL( (time_point_sec{0} -= microseconds{      1LL}), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{1} -= microseconds{1000000LL}), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{2} -= microseconds{1000000LL}), time_point_sec{1} );
   BOOST_CHECK_EQUAL( (time_point_sec{3} -= microseconds{1000000LL}), time_point_sec{2} );

   // ---------------------------------------
   // time_point_sec operator+(uint32_t)const
   BOOST_CHECK_EQUAL( (time_point_sec{0} + u32min), time_point_sec{u32min} );
   BOOST_CHECK_EQUAL( (time_point_sec{0} + u32max), time_point_sec{u32max} );

   // ---------------------------------------
   // time_point_sec operator-(uint32_t)const
   BOOST_CHECK_EQUAL( (time_point_sec{u32min} - u32min), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{u32max} - u32max), time_point_sec{0} );
   BOOST_CHECK_EQUAL( (time_point_sec{u32max} - u32min), time_point_sec{u32max} );

   // --------------------------------------------------------------------
   // friend bool operator==(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{1} == time_point_sec{1}), true  );
   BOOST_CHECK_EQUAL( (time_point_sec{0} == time_point_sec{1}), false );

   // --------------------------------------------------------------------
   // friend bool operator!=(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{1} != time_point_sec{1}), false );
   BOOST_CHECK_EQUAL( (time_point_sec{0} != time_point_sec{1}), true  );

   // -------------------------------------------------------------------
   // friend bool operator<(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{0} <  time_point_sec{1}), true  );
   BOOST_CHECK_EQUAL( (time_point_sec{1} <  time_point_sec{1}), false );

   // --------------------------------------------------------------------
   // friend bool operator<=(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{1} <= time_point_sec{1}), true  );
   BOOST_CHECK_EQUAL( (time_point_sec{2} <= time_point_sec{1}), false );

   // -------------------------------------------------------------------
   // friend bool operator>(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{1} >  time_point_sec{0}), true  );
   BOOST_CHECK_EQUAL( (time_point_sec{1} >  time_point_sec{1}), false );

   // --------------------------------------------------------------------
   // friend bool operator>=(const time_point_sec&, const time_point_sec&)
   BOOST_CHECK_EQUAL( (time_point_sec{1} >= time_point_sec{1}), true  );
   BOOST_CHECK_EQUAL( (time_point_sec{1} >= time_point_sec{2}), false );
   std::cout << "time_point_sec_type_test is done" << std::endl;
} FC_LOG_AND_RETHROW();
*/
BOOST_AUTO_TEST_CASE(duration_to_string_test) try {
   using namespace std::chrono_literals;
   #define TIME_OUTPUT_TEST(TIME, OUTPUT) BOOST_CHECK_EQUAL( to_string(TIME), OUTPUT );

   TIME_OUTPUT_TEST( fc::microseconds(   0 ),   "0 microsec" );
   TIME_OUTPUT_TEST( fc::microseconds(   1 ),   "1 microsec" );
   TIME_OUTPUT_TEST( fc::microseconds(  10 ),  "10 microsec" );
   TIME_OUTPUT_TEST( fc::milliseconds(   0 ),   "0 millisec" );
   TIME_OUTPUT_TEST( fc::milliseconds( 100 ), "100 millisec" );
   TIME_OUTPUT_TEST( fc::seconds     (   0 ),   "0 sec"      );
   TIME_OUTPUT_TEST( fc::seconds     ( 222 ), "222 sec"      );
   TIME_OUTPUT_TEST( fc::minutes     (   0 ),   "0 min"      );
   TIME_OUTPUT_TEST( fc::minutes     (  17 ),  "17 min"      );
   TIME_OUTPUT_TEST( fc::hours       (   0 ),   "0 hours"    );
   TIME_OUTPUT_TEST( fc::hours       (  42 ),  "42 hours"    );
   TIME_OUTPUT_TEST( fc::days        (   0 ),   "0 days"     );
   TIME_OUTPUT_TEST( fc::days        ( 256 ), "256 days"     );
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(time_point_from_string_test) try {
   time_point res;

   for (const auto & tp : { time_point{ chrono::microseconds(          0  ) },
                            time_point{ chrono::microseconds(          1  ) },
                            time_point{ chrono::microseconds(      0x100  ) },
                            time_point{ chrono::microseconds(    0x10000  ) },
                            time_point{ chrono::microseconds(  0x1000000  ) },
                            time_point{ chrono::microseconds( 0x7fffffffU ) },
                            time_point{ chrono::microseconds( 0x80000000U ) },
                            time_point{ chrono::microseconds( 0xc0000000U ) },
                            time_point{ chrono::microseconds( 0x2345678901234llU ) } } )
   {
      auto tp_msec = fc::time_point_cast<fc::milliseconds>(tp);
      BOOST_CHECK_EQUAL( tp_msec, from_iso_string<fc::time_point>( to_iso_string(tp) ) );
      from_iso_string( to_iso_string(tp), res );
      BOOST_CHECK_EQUAL(tp_msec, res);
   }
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(time_point_sec_from_string_test) try {
   time_point_sec res;

   for (const auto & tp : { time_point_sec{ chrono::seconds(          0  ) },
                            time_point_sec{ chrono::seconds(      0x100  ) },
                            time_point_sec{ chrono::seconds(          1  ) },
                            time_point_sec{ chrono::seconds(    0x10000  ) },
                            time_point_sec{ chrono::seconds(  0x1000000  ) },
                            time_point_sec{ chrono::seconds( 0x7fffffffU ) },
                            time_point_sec{ chrono::seconds( 0x80000000U ) },
                            time_point_sec{ chrono::seconds( 0xc0000000U ) } } )
   {
      BOOST_CHECK_EQUAL(tp, from_iso_string<fc::time_point_sec>( to_iso_string(tp) ));
      from_iso_string( to_iso_string(tp), res );
      BOOST_CHECK_EQUAL(tp, res);
   }
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(duration_from_string_test) try {
   using namespace std::chrono_literals;

   #define TIME_INPUT_TEST(TYPE, VALUE, SUFFIX, SPACE) \
      BOOST_CHECK_EQUAL( \
         from_string<TYPE>(SPACE + to_string(VALUE) + SPACE + SUFFIX + SPACE), \
         TYPE{VALUE} );

   #define TIME_INPUT_TEST_WRAPPER(TYPE, VALUE, SPACE) \
      for (auto & suffix : TYPE ## _suffix) { TIME_INPUT_TEST( TYPE, VALUE, suffix, SPACE ); }

   uint64_t values[] = {0, 1, 10, 12345};
   std::string  spaces[] = {"", " ", "\t", "\t\t",  " \t  "};
/*
   for (auto & space : spaces) {
      for (auto & value : values) {
         TIME_INPUT_TEST_WRAPPER( microseconds, value, space );
         TIME_INPUT_TEST_WRAPPER( milliseconds, value, space );
         TIME_INPUT_TEST_WRAPPER( seconds,      value, space );
         TIME_INPUT_TEST_WRAPPER( minutes,      value, space );
         TIME_INPUT_TEST_WRAPPER( hours,        value, space );
         TIME_INPUT_TEST_WRAPPER( days,         value, space );
         TIME_INPUT_TEST_WRAPPER( weeks,        value, space );
      }
   }

   TIME_INPUT_TEST( fc::microseconds,   0,   "microsec" );
   TIME_INPUT_TEST( fc::microseconds,   5,   " microsec" );
   TIME_INPUT_TEST( fc::microseconds,   1,   " microsec" );
   TIME_INPUT_TEST( fc::microseconds,  10,  " microsec" );
   TIME_INPUT_TEST( fc::milliseconds,   0,   " millisec" );
   TIME_INPUT_TEST( fc::milliseconds, 100, " millisec" );
   TIME_INPUT_TEST( fc::seconds,        0,   " sec"      );
   TIME_INPUT_TEST( fc::seconds,      222, " sec"      );
   TIME_INPUT_TEST( fc::minutes,        0,   " min"      );
   TIME_INPUT_TEST( fc::minutes,       17,  " min"      );
   TIME_INPUT_TEST( fc::hours,          0,   " hours"    );
   TIME_INPUT_TEST( fc::hours,         42,  " hours"    );
   TIME_INPUT_TEST( fc::days,           0,   " days"     );
   TIME_INPUT_TEST( fc::days,         256, " days"     );
   */
} FC_LOG_AND_RETHROW();
/*
BOOST_AUTO_TEST_CASE(time_point_shift_operators_test) try {
   #define CHECK_SHIFT(TIME_POINT, VALUE, LITERAL) {\
      std::stringstream ss, orig; \
      auto dur = TIME_POINT::duration(0x ## VALUE ## LITERAL); \
      orig << std::hex << std::setw( sizeof( typename TIME_POINT::rep ) * 2 ) << dur.count(); \
      BOOST_CHECK_EQUAL( ( ss << ( TIME_POINT(dur) ) ).str(), orig.str() ); \
      std::cout << orig.str(); \
      TIME_POINT res_tp; \
      ss >> res_tp; \
      BOOST_CHECK_EQUAL( dur, res_tp.time_since_epoch() ); \
   }

   std::chrono::hours::rep

   CHECK_SHIFT(fc::time_point, 100, us);
   CHECK_SHIFT(fc::time_point, 100, s);
   CHECK_SHIFT(fc::time_point, 100, h);
   CHECK_SHIFT(fc::time_point, 12345, h);
   CHECK_SHIFT(fc::time_point, fffffffffffffff, us);
   CHECK_SHIFT(fc::time_point, 1fffffffffffffff, us);
   CHECK_SHIFT(fc::time_point, efffffffffffffff, us);
   CHECK_SHIFT(fc::time_point, fffffffffffffffe, us);
   CHECK_SHIFT(fc::time_point, ffffffffffffffff, us);

   CHECK_SHIFT(fc::time_point_sec, 100, s);
   CHECK_SHIFT(fc::time_point_sec, 100, h);
   CHECK_SHIFT(fc::time_point_sec, 12345, h);
   CHECK_SHIFT(fc::time_point_sec, ffffffffffffffff, s);

} FC_LOG_AND_RETHROW();

*/
BOOST_AUTO_TEST_CASE(time_point_sec_comparison_test) try {
    time_point_sec tp0   {chrono::seconds(          0  )};
    time_point_sec tp256 {chrono::seconds(      0x100  )};
    time_point_sec tp1   {chrono::seconds(          1  )};
    time_point_sec tp64k {chrono::seconds(    0x10000  )};
    time_point_sec tp16m {chrono::seconds(  0x1000000  )};
    time_point_sec tp2gm1{chrono::seconds( 0x7fffffffU )};
    time_point_sec tp2g  {chrono::seconds( 0x80000000U )};
    time_point_sec tp3g  {chrono::seconds( 0xc0000000U )};

    BOOST_CHECK( tp0 == time_point_sec() );
    BOOST_CHECK( tp0 < tp1 );
    BOOST_CHECK( tp0 < tp256 );
    BOOST_CHECK( tp0 < tp64k );
    BOOST_CHECK( tp0 < tp16m );
    BOOST_CHECK( tp0 < tp2gm1 );
    BOOST_CHECK( tp0 < tp2g );
    BOOST_CHECK( tp0 < tp3g );
    BOOST_CHECK( tp1 > tp0 );
    BOOST_CHECK( tp1 < tp256 );
    BOOST_CHECK( tp1 < tp64k );
    BOOST_CHECK( tp1 < tp16m );
    BOOST_CHECK( tp1 < tp2gm1 );
    BOOST_CHECK( tp1 < tp2g );
    BOOST_CHECK( tp1 < tp3g );
    BOOST_CHECK( tp256 > tp0 );
    BOOST_CHECK( tp256 > tp1 );
    BOOST_CHECK( tp256 < tp64k );
    BOOST_CHECK( tp256 < tp16m );
    BOOST_CHECK( tp256 < tp2gm1 );
    BOOST_CHECK( tp256 < tp2g );
    BOOST_CHECK( tp256 < tp3g );
    BOOST_CHECK( tp64k > tp0 );
    BOOST_CHECK( tp64k > tp1 );
    BOOST_CHECK( tp64k > tp256 );
    BOOST_CHECK( tp64k < tp16m );
    BOOST_CHECK( tp64k < tp2gm1 );
    BOOST_CHECK( tp64k < tp2g );
    BOOST_CHECK( tp64k < tp3g );
    BOOST_CHECK( tp16m > tp0 );
    BOOST_CHECK( tp16m > tp1 );
    BOOST_CHECK( tp16m > tp256 );
    BOOST_CHECK( tp16m > tp64k );
    BOOST_CHECK( tp16m < tp2gm1 );
    BOOST_CHECK( tp16m < tp2g );
    BOOST_CHECK( tp16m < tp3g );
    BOOST_CHECK( tp2gm1 > tp0 );
    BOOST_CHECK( tp2gm1 > tp1 );
    BOOST_CHECK( tp2gm1 > tp256 );
    BOOST_CHECK( tp2gm1 > tp64k );
    BOOST_CHECK( tp2gm1 > tp16m );
    BOOST_CHECK( tp2gm1 < tp2g );
    BOOST_CHECK( tp2gm1 < tp3g );
    BOOST_CHECK( tp2g > tp0 );
    BOOST_CHECK( tp2g > tp1 );
    BOOST_CHECK( tp2g > tp256 );
    BOOST_CHECK( tp2g > tp64k );
    BOOST_CHECK( tp2g > tp16m );
    BOOST_CHECK( tp2g > tp2gm1 );
    BOOST_CHECK( tp2g < tp3g );
    BOOST_CHECK( tp3g > tp0 );
    BOOST_CHECK( tp3g > tp1 );
    BOOST_CHECK( tp3g > tp256 );
    BOOST_CHECK( tp3g > tp64k );
    BOOST_CHECK( tp3g > tp2gm1 );
    BOOST_CHECK( tp3g > tp2g );
    BOOST_CHECK( tp3g > tp16m );
} FC_LOG_AND_RETHROW();


BOOST_AUTO_TEST_SUITE_END()
