#define BOOST_TEST_MODULE io
#include <boost/test/included/unit_test.hpp>

#include <fc/io/cfile.hpp>

using namespace fc;

BOOST_AUTO_TEST_SUITE(cfile_test_suite)
   BOOST_AUTO_TEST_CASE(test_simple)
   {
      fc::temp_directory tempdir;

      cfile t;
      t.set_file_path( tempdir.path() / "test" );
      t.open( "ab+" );
      BOOST_CHECK( t.is_open() );
      BOOST_CHECK( fc::exists( tempdir.path() / "test") );

      t.open( "rb+" );
      BOOST_CHECK( t.is_open() );
      t.write( "abc", 3 );
      BOOST_CHECK_EQUAL( t.tellp(), 3 );
      std::vector<char> v(3);
      t.seek( 0 );
      BOOST_CHECK_EQUAL( t.tellp(), 0 );
      t.read( &v[0], 3 );

      BOOST_CHECK_EQUAL( v[0], 'a' );
      BOOST_CHECK_EQUAL( v[1], 'b' );
      BOOST_CHECK_EQUAL( v[2], 'c' );

      t.seek_end( -2 );
      BOOST_CHECK_EQUAL( t.tellp(), 1 );
      t.read( &v[0], 1 );
      BOOST_CHECK_EQUAL( v[0], 'b' );

      int x = 42, y = 0;
      t.seek( 1 );
      t.write( reinterpret_cast<char*>( &x ), sizeof( x ) );
      t.seek( 1 );
      t.read( reinterpret_cast<char*>( &y ), sizeof( y ) );
      BOOST_CHECK_EQUAL( x, y );

      t.close();
      BOOST_CHECK( !t.is_open() );

      // re-open and read again
      t.open( "rb+" );
      BOOST_CHECK( t.is_open() );

      y = 0;
      t.seek( 1 );
      t.read( reinterpret_cast<char*>( &y ), sizeof( y ) );
      BOOST_CHECK_EQUAL( x, y );

      t.close();
      fc::remove_all( t.get_file_path() );
      BOOST_CHECK( !fc::exists( tempdir.path() / "test") );
   }

BOOST_AUTO_TEST_SUITE_END()
