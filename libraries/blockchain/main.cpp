#include <eosio/blockchain/controller.hpp>
#include <eosio/blockchain/db.hpp>
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
      controller control( ios );

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
         auto tbl = sco.create_table<key_value_object>( N(test) );
      }


   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   } catch ( const std::exception& e ) {
      elog(e.what());
   }

   return 0;
}

#if 0
      /*
      db.undo_all_blocks();
      auto b = db.start_block();
      auto s = b.start_shard( {1} )
      auto t = s.start_transaction();
      auto tbl = t.create_table<...>()
      t.squash();
      t.undo();
      s.start_transaction()





      wlog( "undoing all pending state" );
      db.undo_all();
      wlog( "setting revision to 100" );
      db.set_revision( 100 );

      {
         auto ses   = db.start_session(101);

         const database& cdb = db;
         const auto* s = cdb.find_scope(1);
         if( !s ) {
            wlog( "creating scope 1" );
            ses.create_scope(1);
         }
         auto shar = ses.start_shard( {1} );

         shar.start_session

         auto okv_table = shar.find_table<key_value_object>(1,1000);
         if( !okv_table ) {
            elog("creating table" );
            okv_table = shar.create_table<key_value_object>( 1, 1000 );
            elog( "finding table" );
            auto test_find = shar.find_table<key_value_object>(1,1000);
            FC_ASSERT( test_find, "expected to find table just created" );
         }

         ilog( "emplacing object" );
         const auto& obj = okv_table->emplace( [&]( key_value_object& o ) {
             o.key = fc::time_point::now().sec_since_epoch();
             o.value = 2;
         });

         wdump(("before")(obj.value));
         okv_table->modify( obj, [&]( auto& o ) {
            o.value++;
         });

         wdump(("after")(obj.value));

         okv_table->remove(obj);

         ses.push();
      }
      ilog( "commit revision 102" );
      db.commit_revision(102);
      idump((db.revision()));



//      /*auto scope = */ses.create_scope( 1 );
  //    auto scope = shard.get_scope( 1 );
   //   auto table = shard.create_table<key_value_object>( scope );

      /*
      const auto& row = table.find( key );
         table.modify( row, [&]( auto& r ){
      });

      auto session = shard.start_undo_session();

      session.squash();
      session.undo();
      */


      /*

      wlog( "starting thread with scope 1" );
      auto t1 = db.start_thread( {1} );
      auto& table = t1.create_table< key_value_object >( scope, 3 );

      auto& row = table.emplace( [&]( auto& kv ) {
          kv.key = 12;
          kv.value = 22;
      });

      wlog( "starting revision 110" );
      db.push_revision( 110 );
      {
         wlog( "creating scope 2" );
         auto& scope = db.create_scope( 2 );
         auto t2 = db.start_thread( {2} );
         auto& table = t2.create_table< key_value_object >( scope, 3 );

         const auto& row = t2.create<key_value_object>( scope, table, [&]( auto& obj ){

         });

         t2.modify( table, row, [&]( auto& ) {

         });

         auto& row = table.emplace( [&]( auto& kv ) {
             kv.key = 13;
             kv.value = 23;
         });

         t2.modify( row, [&]( auto& kv ) {
             kv.key = 44;
             kv.key = 45;
         });
      }

      wlog( "starting to pop 110" );
      db.pop_revision( 110 );
      wlog( "now popping 100" );
      db.pop_revision( 100 );
      wlog( "done popping 100" );
      */

     // db.delete_table( table );
     // db.delete_scope( scope );

      /*
       *  A cycle is an interface through which all changes must be made,
       *  its primary job is to create sessions with non-overlapping scopes.
       *
       * 
       *
       *
      auto cycle = db.start_cycle();
      auto& scope = cycle.create_scope(1);
      auto& table = cycle.create_table< key_value_object >( scope, 3 );

      {
         auto session = cycle.start_session( {1}, true );
            session.emplace( table, [&]( auto& kv ) {
                kv.key = 12;
                kv.value = 22;
            });
         session.squash(); 
      }
      */
      



      //db.create_table< key_value_object >( 1, 2 );

      /**
       *  Each cycle represents a set of modifications which can occur in parallel, from this
       *  cycle you can start undo sessions with non-intersecting scopes.
       *
       *  true indicatest that undo history should be tracked
       */
//      auto cycle = db.start_cycle( true );

      /**
       *  This starts an UNDO session for a set of scopes provided the scopes are not already
       *  in use. In this case the session declares that it neesd test RW and nothing RO and that
       *  undo history should be tracked. This represents the thread scope
       */
//      auto thread_session = cycle.start_session( {N(test)}, {} );

      /** 
       *  Within this thread_session we want to start a child session for the transaction which
       *  might fail and have to be undone.
       */
//      auto trx_session = s.start_session();  

      /**
       *  Now we can get the table for anything that is in the scope of the session, we specify the
       *  table by its given name.  This table must have been previously created and the type of
       *  data stored (key_value) must match the type it was created with or it will throw.
       */
//      auto keyvalue_table = trx_session.get_table<key_value>( N(test), N(account) );

      /**
       *  This will modify the table and update the undostate on the table, adding it to the
       *  session to be tracked.
       */
      /*
      const key_value& newrow = keyvalue_table.create( [&]( key_value& r ) {
         r.key = 1;
         r.value = 2;
      });
      */

      /**
       *  For every table that was modified by this session, merge its undo data with
       *  the thread_session.  Note the set of tables in the trx_session may be different
       *  than the set of tables in the parent.
       */
//      trx_session.squash(); // trx_session.undo() 
//      thread_session.push(); /// prevent it from being undone

#endif
