#define BOOST_TEST_MODULE chainbase test

#include <boost/test/unit_test.hpp>
#include <chainbase/chainbase.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <iostream>

using namespace chainbase;
using namespace boost::multi_index;

//BOOST_TEST_SUITE( serialization_tests, clean_database_fixture )

struct book : public chainbase::object<0, book> {

   template<typename Constructor, typename Allocator>
    book(  Constructor&& c, Allocator&& a ) {
       c(*this);
    }

    id_type id;
    int a = 0;
    int b = 1;
};

typedef multi_index_container<
  book,
  indexed_by<
     ordered_unique< member<book,book::id_type,&book::id> >,
     ordered_unique< BOOST_MULTI_INDEX_MEMBER(book,int,a) >,
     ordered_unique< BOOST_MULTI_INDEX_MEMBER(book,int,b) >
  >,
  chainbase::node_allocator<book>
> book_index;

CHAINBASE_SET_INDEX_TYPE( book, book_index )


BOOST_AUTO_TEST_CASE( open_and_create ) {
   boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
   try {
      std::cerr << temp << " \n";

      chainbase::database db(temp, database::read_write, 1024*1024*8);
      chainbase::database db2(temp, database::read_only, 0, true); /// open an already created db
      BOOST_CHECK_THROW( db2.add_index< book_index >(), std::runtime_error ); /// index does not exist in read only database

      db.add_index< book_index >();
      BOOST_CHECK_THROW( db.add_index<book_index>(), std::logic_error ); /// cannot add same index twice


      db2.add_index< book_index >(); /// index should exist now


      BOOST_TEST_MESSAGE( "Creating book" );
      const auto& new_book = db.create<book>( []( book& b ) {
          b.a = 3;
          b.b = 4;
      } );
      const auto& copy_new_book = db2.get( book::id_type(0) );
      BOOST_REQUIRE( &new_book != &copy_new_book ); ///< these are mapped to different address ranges

      BOOST_REQUIRE_EQUAL( new_book.a, copy_new_book.a );
      BOOST_REQUIRE_EQUAL( new_book.b, copy_new_book.b );

      db.modify( new_book, [&]( book& b ) {
          b.a = 5;
          b.b = 6;
      });
      BOOST_REQUIRE_EQUAL( new_book.a, 5 );
      BOOST_REQUIRE_EQUAL( new_book.b, 6 );

      BOOST_REQUIRE_EQUAL( new_book.a, copy_new_book.a );
      BOOST_REQUIRE_EQUAL( new_book.b, copy_new_book.b );

      {
          auto session = db.start_undo_session(true);
          db.modify( new_book, [&]( book& b ) {
              b.a = 7;
              b.b = 8;
          });

         BOOST_REQUIRE_EQUAL( new_book.a, 7 );
         BOOST_REQUIRE_EQUAL( new_book.b, 8 );
      }
      BOOST_REQUIRE_EQUAL( new_book.a, 5 );
      BOOST_REQUIRE_EQUAL( new_book.b, 6 );

      {
          auto session = db.start_undo_session(true);
          const auto& book2 = db.create<book>( [&]( book& b ) {
              b.a = 9;
              b.b = 10;
          });

         BOOST_REQUIRE_EQUAL( new_book.a, 5 );
         BOOST_REQUIRE_EQUAL( new_book.b, 6 );
         BOOST_REQUIRE_EQUAL( book2.a, 9 );
         BOOST_REQUIRE_EQUAL( book2.b, 10 );
      }
      BOOST_CHECK_THROW( db2.get( book::id_type(1) ), std::out_of_range );
      BOOST_REQUIRE_EQUAL( new_book.a, 5 );
      BOOST_REQUIRE_EQUAL( new_book.b, 6 );


      {
          auto session = db.start_undo_session(true);
          db.modify( new_book, [&]( book& b ) {
              b.a = 7;
              b.b = 8;
          });

         BOOST_REQUIRE_EQUAL( new_book.a, 7 );
         BOOST_REQUIRE_EQUAL( new_book.b, 8 );
         session.push();
      }
      BOOST_REQUIRE_EQUAL( new_book.a, 7 );
      BOOST_REQUIRE_EQUAL( new_book.b, 8 );
      db.undo();
      BOOST_REQUIRE_EQUAL( new_book.a, 5 );
      BOOST_REQUIRE_EQUAL( new_book.b, 6 );

      BOOST_REQUIRE_EQUAL( new_book.a, copy_new_book.a );
      BOOST_REQUIRE_EQUAL( new_book.b, copy_new_book.b );
   } catch ( ... ) {
      bfs::remove_all( temp );
      throw;
   }
   bfs::remove_all( temp );
}

// BOOST_AUTO_TEST_SUITE_END()
