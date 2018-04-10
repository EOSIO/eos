#include <eosio/chain/controller.hpp>
#include <chainbase/database.hpp>

namespace eosio { namespace chain {

struct pending_state {
   pending_state( database::session&& s )
   :_db_session(s){}

   block_timestamp_type           _pending_time;
   database::session              _db_session;
   vector<transaction_metadata>   _transaction_metas;
};

struct controller_impl {
   chainbase::database            db;
   optional<pending_state>        pending;
   block_state_ptr                head; 
   fork_database                  fork_db;            

   wasm_interface                 _wasm_interface;


   controller_impl() {
      initialize_indicies();
   }

   void initialize_indicies() {

   }

   void abort_pending_block();
};



controller::controller():my( new controller_impl() )
{
}

controller::~controller() {
}


const chainbase::database& controller::db()const { return my->db; }


block_state_ptr             controller::push_block( const signed_block_ptr& b ) {

}

transaction_trace           controller::push_transaction( const signed_transaction& t ) {

}

optional<transaction_trace> controller::push_deferred_transaction() {

}

   
} } /// eosio::chain
