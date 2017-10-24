#include <eosio/blockchain/controller.hpp>
#include <eosio/blockchain/database.hpp>
#include <fc/log/logger.hpp>

using namespace eosio::blockchain;
using namespace boost::multi_index;

namespace eosio { namespace blockchain {

struct by_id;

struct key_value_object : public object< 2, key_value_object >  {
   template<typename Constructor, typename Allocator>
   key_value_object( Constructor&& c, Allocator&& ){
      ilog( "create key_value_object ${p}", ("p",int64_t(this)) );
      c(*this);
   }
   ~key_value_object() {
      ilog( "destroy key_value_object ${p}", ("p",int64_t(this)) );
      idump((key)(value));
   }
   id_type id;
   int key;
   int value;
};

struct by_key;
typedef shared_multi_index_container< key_value_object,
   indexed_by<
      ordered_unique< tag<by_id>, member<key_value_object, key_value_object::id_type, &key_value_object::id > >,
      ordered_unique< tag<by_key>, member<key_value_object, int, &key_value_object::key > >
   >
> key_value_index;

} } /// eosio::blockchain 

EOSIO_SET_INDEX_TYPE( eosio::blockchain::key_value_object, eosio::blockchain::key_value_index );

int main( int argc, char** argv ) {
   try {
      boost::asio::io_service ios;
      controller control;
      control.open_database( "datadir", 1024*1024 );

      /*
      register_table< key_value_object >();

      wlog( "loading database" );
      database db( "test.db", 1024*1024 );

      idump((db.last_reversible_block()));

      if( db.last_reversible_block() == database::max_block_num ) {
         auto blk = db.start_block(1);
         auto cyc = blk.start_cycle();
         auto shr = cyc.start_shard( {N(scope)} );
         auto trx = shr.start_transaction();
         auto sco = trx.create_scope( N(dan) );
         auto tbl = sco.create_table<key_value_object>( {N(owner),N(test)} );
      }
      */


   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   } catch ( const std::exception& e ) {
      elog(e.what());
   }

   return 0;
}

