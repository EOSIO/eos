/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */
#include <cmath>

#include <eosiolib/eosio.hpp>
#include <eosiolib/datastream.hpp>

#include "test_api.hpp"

template <typename T>
struct testtype {
    static void run( const T &v, const char *errmsg = "" )  {
        char buf[128];
        eosio::datastream<char *> ds( buf, sizeof(buf) );
        ds << v;
        T v2;
        ds.seekp(0);
        ds >> v2;
        eosio_assert ( v == v2, errmsg );   
    }
};

template <>
struct testtype<double> {
   static void run( const double &v, const char *errmsg = "" ) {
      char buf[128];
      eosio::datastream<char *> ds( buf, sizeof(buf) );
      ds << v;
      double v2;
      ds.seekp(0);
      ds >> v2;
      eosio_assert( std::abs(v - v2) < 1e-20, errmsg );
   }
};

template <>
struct testtype<float> {
   static void run( const float &v, const char *errmsg = "" ) {
      char buf[128];
      eosio::datastream<char *> ds( buf, sizeof(buf) );
      ds << v;
      float v2;
      ds.seekp(0);
      ds >> v2;
      eosio_assert( std::abs(v - v2) < float(1e-10), errmsg );
   }
};

void test_datastream::test_basic()
{

    testtype<bool>::run( true, "bool" );
    testtype<bool>::run( false, "bool" );
    testtype<char>::run( -123, "int8" );
    testtype<unsigned char>::run( 127, "uint8" );
    testtype<short>::run( -12345, "int16" );
    testtype<unsigned short>::run( 12345, "uint16" );
    testtype<int>::run( -1234567890, "int32" );
    testtype<unsigned int>::run( 3234567890u, "uint32" );
    testtype<long long>::run( (long long)0x8000000000000000ll, "int64" );
    testtype<unsigned long long>::run( 0x7fffffffffffffffull, "uint64" );
    testtype<float>::run( 1.234f, "float" );
    testtype<double>::run( 0.333333333333333333, "double" );

    // this should generate compile error
    //testtype<char *>::run((char *)0x12345678, "pointer");

    struct Pair {
        int a;
        double d;
          bool operator==( const Pair &p ) const { return a == p.a && std::abs(d - p.d) < 1e-20; } 
    };
    testtype<Pair>::run({ 1, 1.23456}, "struct" );

    struct StaticArray {
        int a[2];
        bool operator==( const StaticArray &o ) const { return a[0] == o.a[0] && a[1] == o.a[1]; }
    };
    testtype<StaticArray>::run( {{10,20}}, "StaticArray" );

    testtype<std::string>::run( "hello", "string" );

    testtype<std::vector<int> >::run( {10,20,30}, "vector" );
    testtype<std::vector<int> >::run( {}, "empty vector" );
    testtype<std::array<int, 3> >::run( {{10,20,30}}, "std::array<T,N>" );
    testtype<std::map<int, std::string> >::run( {{1,"apple"}, {2,"cat"}, {3,"panda"}}, "map" );
    testtype<std::tuple<int, std::string, double> >::run( {1, "abc", 3.3333}, "tuple" );
}

