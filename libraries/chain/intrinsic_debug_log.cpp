/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/intrinsic_debug_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fstream>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace eosio { namespace chain {

   namespace detail {
      class intrinsic_debug_log_impl {
         public:
            enum class state_t {
               closed,
               read_only,
               waiting_to_start_block,
               in_block,
               in_transaction,
               in_action
            };

            static const uint8_t block_tag       = 0;
            static const uint8_t transaction_tag = 1;
            static const uint8_t action_tag      = 2;
            static const uint8_t intrinsic_tag   = 3;

            std::fstream             log;
            bfs::path                log_path;
            state_t                  state = state_t::closed;
            uint64_t                 last_commited_block_pos = 0;
            uint64_t                 most_recent_block_pos = 0;
            uint32_t                 most_recent_block_num = 0;
            uint32_t                 intrinsic_counter = 0;
            bool                     written_start_block = false;

            void open( bool read_only, bool start_fresh );
            void close();

            void start_block( uint32_t block_num );
            void ensure_block_start_written();
            void finish_block();
      };

      void intrinsic_debug_log_impl::open( bool read_only, bool start_fresh ) {
         FC_ASSERT( !log.is_open(), "cannot open when the log is already open" );
         FC_ASSERT( state == state_t::closed, "state out of sync" );

         std::ios_base::openmode open_mode = std::ios::in | std::ios::binary;

         if( !read_only ) {
            open_mode |= (std::ios::out | std::ios::app);
         } else {
            FC_ASSERT( !start_fresh, "start_fresh cannot be true in read-only mode" );
         }

         if( start_fresh ) {
            open_mode |= std::ios::trunc;
         }

         log.open( log_path.generic_string().c_str(), open_mode );
         state = state_t::waiting_to_start_block;

         log.seekg( 0, std::ios::end );
         auto file_size = log.tellg();
         if( file_size > 0 ) {
            log.seekg( -sizeof(last_commited_block_pos), std::ios::end );
            log.read( (char*)&last_commited_block_pos, sizeof(last_commited_block_pos) );
            FC_ASSERT( last_commited_block_pos < file_size, "corrupted log: invalid block pos" );
            most_recent_block_pos = last_commited_block_pos;

            log.seekg( last_commited_block_pos, std::ios::beg	);
            uint8_t tag = 0;
            log.read( (char*)&tag, sizeof(tag) );
            FC_ASSERT( tag == block_tag, "corrupted log: expected block tag" );

            log.read( (char*)&most_recent_block_num, sizeof(most_recent_block_num) );
         }
         log.seekg( 0, std::ios::beg );
      }

      void intrinsic_debug_log_impl::close() {
         if( !log.is_open() ) return;
         FC_ASSERT( state != state_t::closed, "state out of sync" );

         log.flush();
         log.close();
         if( state != state_t::waiting_to_start_block ) {
            bfs::resize_file( log_path, last_commited_block_pos );
         }
         state = state_t::closed;
         intrinsic_counter = 0;
         written_start_block = false;
      }

      void intrinsic_debug_log_impl::start_block( uint32_t block_num  ) {
         FC_ASSERT( state == state_t::waiting_to_start_block, "cannot start block while still processing a block" );
         FC_ASSERT( most_recent_block_num < block_num,
                    "must start a block with greater height than previously started blocks" );

         state = state_t::in_block;
         most_recent_block_pos = log.tellg();
         most_recent_block_num = block_num;
      }

      void intrinsic_debug_log_impl::ensure_block_start_written() {
         if( written_start_block ) return;
         FC_ASSERT( state == state_t::in_block, "called while in invalid state" );
         log.put( block_tag );
         log.write( (char*)&most_recent_block_num, sizeof(most_recent_block_num) );
         written_start_block = true;
      }

      void intrinsic_debug_log_impl::finish_block() {
         if( written_start_block ) {
            log.put( block_tag );
            log.write( (char*)&most_recent_block_pos, sizeof(most_recent_block_pos) );
         }
         state = state_t::waiting_to_start_block;
         intrinsic_counter = 0;
         written_start_block = false;
      }
   }

   intrinsic_debug_log::intrinsic_debug_log( const bfs::path& log_path )
   :my( new detail::intrinsic_debug_log_impl() ) {
      my->log_path = log_path;
      my->log.exceptions( std::fstream::failbit | std::fstream::badbit );
   }

   intrinsic_debug_log::intrinsic_debug_log( intrinsic_debug_log&& other) {
      my = std::move(other.my);
   }

   intrinsic_debug_log::~intrinsic_debug_log() {
      if( my ) {
         my->close();
         my.reset();
      }
   }

   void intrinsic_debug_log::open( open_mode mode ) {
      bool read_only = (mode == open_mode::read_only);
      my->open( read_only, ( read_only ? false : (mode == open_mode::create_new) ) );
   }

   void intrinsic_debug_log::close() {
      my->close();
   }

   void intrinsic_debug_log::flush() {
      my->log.flush();
   }

   void intrinsic_debug_log::start_block( uint32_t block_num ) {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      my->start_block( block_num );
   }

   void intrinsic_debug_log::finish_block() {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      my->finish_block();
   }

   void intrinsic_debug_log::start_transaction( const transaction_id_type& trx_id ) {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::closed
                 && my->state != detail::intrinsic_debug_log_impl::state_t::waiting_to_start_block,
                 "cannot start transaction in the current state" );

      char buffer[32];
      fc::datastream<char*> ds( buffer, sizeof(buffer) );
      fc::raw::pack(ds, trx_id);
      FC_ASSERT( !ds.remaining(), "unexpected size for transaction id" );

      my->ensure_block_start_written();
      my->log.put( detail::intrinsic_debug_log_impl::transaction_tag );
      my->log.write( buffer, sizeof(buffer) );

      my->state = detail::intrinsic_debug_log_impl::state_t::in_transaction;
      my->intrinsic_counter = 0;
   }

   void intrinsic_debug_log::start_action( uint64_t global_sequence_num ) {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::in_transaction
                 || my->state == detail::intrinsic_debug_log_impl::state_t::in_action,
                 "cannot start action in the current state" );

      my->log.put( detail::intrinsic_debug_log_impl::action_tag );
      my->log.write( (char*)&global_sequence_num, sizeof(global_sequence_num) );

      my->state = detail::intrinsic_debug_log_impl::state_t::in_action;
      my->intrinsic_counter = 0;
   }

   void intrinsic_debug_log::acknowledge_intrinsic_without_recording() {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::in_action,
                 "can only acknowledge intrinsic within action state" );
      ++(my->intrinsic_counter);
   }

   void intrinsic_debug_log::record_intrinsic( const digest_type& arguments_hash, const digest_type& memory_hash ) {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::in_action,
                 "can only record intrinsic within action state" );

      char buffer[64];
      fc::datastream<char*> ds( buffer, sizeof(buffer) );
      fc::raw::pack(ds, arguments_hash);
      fc::raw::pack(ds, memory_hash);
      FC_ASSERT( !ds.remaining(), "unexpected size for digest" );

      my->log.put( detail::intrinsic_debug_log_impl::intrinsic_tag );
      my->log.write( (char*)&my->intrinsic_counter, sizeof(my->intrinsic_counter) );
      my->log.write( buffer, sizeof(buffer) );

      ++(my->intrinsic_counter);
   }

} } /// eosio::chain
