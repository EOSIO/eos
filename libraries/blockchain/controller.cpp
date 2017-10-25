#include <eosio/blockchain/controller.hpp>
#include <eosio/blockchain/database.hpp>
#include <eosio/blockchain/fork_database.hpp>
#include <eosio/blockchain/block_log.hpp>
#include <eosio/blockchain/global_state_index.hpp>
#include <eosio/blockchain/producer_index.hpp>
#include <eosio/blockchain/config.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace blockchain {

   controller::controller() {
      register_table< global_state_object >();
      register_table< active_producers_object >();
      register_table< producer_object >();

      _ios.reset( new boost::asio::io_service() );
      _thread.reset( new std::thread( [&](){ _ios->run(); } ) );
   }

   controller::~controller() {
     _ios.reset();
     _thread->join();
   }

   void controller::open_database( path datadir, size_t shared_mem_size ) {
      FC_ASSERT( !_db, "database already open at ${datadir}", ("datadir",_datadir) );
      _datadir = datadir;
      _shared_mem_size = shared_mem_size;

      _fork_db.reset( new fork_database() );
      _block_log.reset( new block_log( _datadir / "blocks" ) );

      auto head = _block_log->read_head();
      if( !head ) {
         wlog( "no blockchain found" );
         initialize_state_database();

         const auto& global_state = get_global_state();
         ilog( "head block: ${n} ${i}  ${t}", ("n",global_state.head_block_num)
                                              ("i",global_state.head_block_id)
                                              ("t",global_state.head_block_time) );
      } else {
         open_state_database();
         ilog( "undoing all pending state" );
         _db->undo_until(0);

         const auto& global_state = get_global_state();
         ilog( "head block: ${n} ${i}  ${t}", ("n",global_state.head_block_num)
                                              ("i",global_state.head_block_id)
                                              ("t",global_state.head_block_time) );
      }
   }

   const global_state_object& controller::get_global_state()const {
      const auto& scope = _db->get_scope( N(eosio.block) );
      const auto& t = scope.get_table<global_state_index>( {N(eosio.block), N(global)} );
      return *t.get_index<by_id>().begin();
   }

   void controller::initialize_state_database() { try {
      reset_state_database();
      ilog( "initializing state database" );

      auto blk = _db->start_block(0);
      auto cyc = blk.start_cycle();
      auto shr = cyc.start_shard( {N(eosio.block)} );
      auto trx = shr.start_transaction();

      auto block_scope = trx.create_scope( N(eosio.block) );
      ilog( "get block scope: ${s}", ("s",name(N(eosio.block)))("sh",block_scope.get_name()) );

      auto globaltbl = block_scope.create_table<global_state_object>( {N(eosio.block),N(global)} );
      globaltbl.create( [&]( auto& global ) {
         global.head_block_time = config::initial_blockchain_time;
      });

      auto active_producers_tbl = block_scope.create_table<active_producers_object>( {N(eosio.block),N(activepro)} );
      active_producers_tbl.create( [&]( auto& active ) {
           active.producers.resize( config::producer_count );
           for( auto i = 0; i < config::producer_count; ++i ) {
              active.producers[i] = N(inita) + i;
           }
      });

      auto producertbl = block_scope.create_table<producer_object>( {N(eosio.block),N(producers)} );
      auto first = N(inita);
      for( auto i = 0; i < config::producer_count; ++i ) {
         producertbl.create( [&]( auto& initproducer ) {
             initproducer.producer = first + i;
             initproducer.key      = config::initial_public_key;
         });
      }

      trx.commit();
      blk.commit();

      _db->commit( 0 );
   } FC_CAPTURE_AND_RETHROW() }

   void controller::reset_state_database() // private
   {
      _db.reset();
      fc::remove_all( _datadir / "shared_mem" );
      open_state_database();
   }

   void controller::open_state_database() // private
   {
      _db.reset( new database( _datadir / "shared_mem", _shared_mem_size ) );
   }


} } /// eosio::blockchain
