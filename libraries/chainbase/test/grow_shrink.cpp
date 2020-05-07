#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <chainbase/chainbase.hpp>

using namespace chainbase;

const pinnable_mapped_file::map_mode test_modes[] = {pinnable_mapped_file::map_mode::mapped, pinnable_mapped_file::map_mode::heap};

BOOST_DATA_TEST_CASE(grow_shrink, boost::unit_test::data::make(test_modes), map_mode) {
   boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
   try {
      const size_t db_start_size  = 8u*1024u*1024u;
      const size_t db_grow_size   = 16u*1024u*1024u;
      const size_t db_shrunk_size = 2u*1024u*1024u;

      {
         chainbase::database db(temp, database::read_write, db_start_size, false, map_mode);
      }
      {
         chainbase::database db(temp, database::read_write, db_grow_size, false, map_mode);
	 BOOST_CHECK(db.get_free_memory() > db_start_size);
      }
      {
         chainbase::database db(temp, database::read_write, db_shrunk_size, false, map_mode);
	 BOOST_CHECK(db.get_free_memory() > db_start_size);
      }

   } catch(...) {
      bfs::remove_all(temp);
      throw;
   }
   bfs::remove_all(temp);
}
