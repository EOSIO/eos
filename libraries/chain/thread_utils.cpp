#include <eosio/chain/thread_utils.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

namespace eosio { namespace chain {


//
// named_thread_pool
//
named_thread_pool::named_thread_pool( std::string name_prefix, size_t num_threads, bool one_io_context )
{
   FC_ASSERT( num_threads > 0, "num_threads should not be zero" );
   _ioc_works.emplace();
   for( size_t i = 0; i < num_threads; ++i ) {
      if( !one_io_context || i == 0) {
         _iocs.emplace_back( new boost::asio::io_context );
         _ioc_works->emplace_back( boost::asio::make_work_guard( *_iocs.back() ) );
      }
      _thread_pool.emplace_back([i, name_prefix, &ioc = *_iocs.back()](){
         std::string tn = name_prefix + "-" + std::to_string( i );
         fc::set_os_thread_name( tn );
         ioc.run();
      });
   }
}

boost::asio::io_context& named_thread_pool::get_executor() {
   if( _iocs.size() > 1 ) {
      size_t i = _next_ioc.fetch_add( 1 );
      return *_iocs.at( i % _iocs.size() );
   } else {
      return *_iocs.back();
   }
}

named_thread_pool::~named_thread_pool() {
   stop();
}

void named_thread_pool::stop() {
   try {
      _ioc_works.reset();
      for( auto& ioc : _iocs ) {
         ioc->stop();
      }
      for( auto& t : _thread_pool ) {
         t.join();
      }
      _thread_pool.clear();
      _iocs.clear();
   } FC_LOG_AND_DROP()
}


} } // eosio::chain