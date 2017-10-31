#include <eosio/blockchain/scheduler.hpp>
#include <fc/exception/exception.hpp>

namespace eosio { namespace blockchain {

scheduler::scheduler( uint32_t thread_count  )
:_work(_ios),_on_done([](){}) 
{
   _threads.reserve(thread_count);
   for( uint32_t i = 0; i < thread_count; ++i ) {
      _threads.emplace_back( [this](){ 
         try {
            _ios.run();
         } catch ( ... ){}

         ++_completed;
         if( _threads.size() == _completed ) {
            _on_done();
         }
      });
   }

}

scheduler::~scheduler() {
   _work.reset();
   for( auto& t : _threads )
      t.join();
}

scheduler::strand_handle scheduler::create_strand() {
   FC_ASSERT( !_wait, "cannot post new tasks while waiting for execution to abort" );
   _current_strands.emplace_back( _ios );
   return strand_handle( _current_strands.size()-1, *this );
}

scheduler::strand_handle scheduler::post( lambda_type exec, strand_handle after ) {
   FC_ASSERT( !_wait, "cannot post new tasks while waiting for execution to abort" );
   FC_ASSERT( &after._scheduler == this );

   _current_strands[after._index].post( exec );

   return after;
} /// post

void scheduler::async_wait( lambda_type on_done ) {
   FC_ASSERT( !_wait, "already waiting for completion" );
   FC_ASSERT( _completed != _threads.size(), "threads should still be running" );
   _on_done = on_done;
   _wait = true;
   _work.reset();
}






} } /// eosio::blockchain
