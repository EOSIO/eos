#include <eosio/blockchain/controller.hpp>
#include <eosio/blockchain/database.hpp>

namespace eosio { namespace blockchain {

   controller::controller() {
   }

   controller::~controller() {

   }

   void controller::open_database( path datadir, size_t shared_mem_size ) {
      FC_ASSERT( !_db, "database already open at ${datadir}", ("datadir",_datadir) );
      _datadir = datadir;
      _db.reset( new database( datadir/"shared_mem", shared_mem_size ) );
   }

} } /// eosio::blockchain
