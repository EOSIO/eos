#include <chainbase/chainbase.hpp>
#include <fc/filesystem.hpp>

#include <memory>

namespace eosio { namespace chain { namespace test {

/**
 * Utility class to create and tear down a temporary chainbase::database using RAII
 *
 * @tparam MAX_SIZE - the maximum size of the chainbase::database
 */
template<uint64_t MAX_SIZE>
struct chainbase_fixture {
   chainbase_fixture()
   : _tempdir()
   , _db(std::make_unique<chainbase::database>(_tempdir.path(), chainbase::database::read_write, MAX_SIZE))
   {
   }

   ~chainbase_fixture()
   {
      _db.reset();
      _tempdir.remove();
   }

   fc::temp_directory                    _tempdir;
   std::unique_ptr<chainbase::database>  _db;
};

} } }  // eosio::chain::test