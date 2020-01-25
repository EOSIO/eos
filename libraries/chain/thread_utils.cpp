#include <eosio/chain/thread_utils.hpp>
#include <fc/log/logger_config.hpp>

namespace eosio { namespace chain {


//
// named_thread_pool
//
named_thread_pool::named_thread_pool( std::string name_prefix, size_t num_threads )
: _thread_pool( num_threads )
{
   _ioc_work.emplace( boost::asio::make_work_guard( _ioc ) );
   for( size_t i = 0; i < num_threads; ++i ) {
      boost::asio::post( _thread_pool, [&ioc = _ioc, name_prefix, i]() {
         std::string tn = name_prefix + "-" + std::to_string( i );
         fc::set_os_thread_name( tn );
         ioc.run();
      } );
   }
}

named_thread_pool::~named_thread_pool() {
   stop();
}

void named_thread_pool::stop() {
   _ioc_work.reset();
   _ioc.stop();
   _thread_pool.join();
   _thread_pool.stop();
}


} } // eosio::chain