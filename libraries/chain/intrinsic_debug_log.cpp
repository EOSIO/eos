/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/intrinsic_debug_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <fc/scoped_exit.hpp>
#include <fstream>
#include <map>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace eosio { namespace chain {

   namespace detail {
      struct extended_block_data {
         extended_block_data( intrinsic_debug_log::block_data&& block_data,
                              int64_t pos, int64_t previous_pos, int64_t next_pos )
         :block_data( std::move(block_data) )
         ,pos( pos )
         ,previous_pos( previous_pos )
         ,next_pos( next_pos )
         {}

         intrinsic_debug_log::block_data block_data;
         int64_t pos; //< pos should never be negative
         int64_t previous_pos; //< if previous_pos is -1, then this is first block in the log
         int64_t next_pos; //< if next_pos is -1, then this is the last block in the log
      };

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
            std::map<uint64_t, extended_block_data> block_data_cache;
            state_t                  state = state_t::closed;
            int64_t                  file_size = -1;
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

            int64_t get_position_of_previous_block( int64_t pos );
            const extended_block_data& get_block_at_position( int64_t pos );
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
         state = (read_only ? state_t::read_only : state_t::waiting_to_start_block);

         log.seekg( 0, std::ios::end );
         file_size = log.tellg();
         if( file_size > 0 ) {
            log.seekg( -sizeof(last_commited_block_pos), std::ios::end );
            log.read( (char*)&last_commited_block_pos, sizeof(last_commited_block_pos) );
            FC_ASSERT( last_commited_block_pos < file_size, "corrupted log: invalid block position" );
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
         if( state != state_t::read_only && state != state_t::waiting_to_start_block ) {
            bfs::resize_file( log_path, last_commited_block_pos );
         }
         state = state_t::closed;
         intrinsic_counter = 0;
         written_start_block = false;
         file_size = -1;

         block_data_cache.clear();
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

      int64_t intrinsic_debug_log_impl::get_position_of_previous_block( int64_t pos ) {
         int64_t previous_pos = -1;
         if( pos > sizeof(last_commited_block_pos) ) {
            decltype(last_commited_block_pos) read_pos{};
            log.seekg( pos - sizeof(read_pos), std::ios::beg );
            log.read( (char*)&read_pos, sizeof(read_pos) );
            FC_ASSERT( read_pos < log.tellg(), "corrupted log: invalid block position" );
            previous_pos = read_pos;
         }
         return previous_pos;
      }

      const extended_block_data& intrinsic_debug_log_impl::get_block_at_position( int64_t pos ) {
         FC_ASSERT( state == state_t::read_only, "only allowed in read-only mode" );
         FC_ASSERT( 0 <= pos && pos < file_size, "position is out of bounds" );

         auto itr = block_data_cache.find( static_cast<uint64_t>(pos) );
         if( itr != block_data_cache.end() ) return itr->second;

         int64_t previous_pos = get_position_of_previous_block( pos );
         log.seekg( pos, std::ios::beg );

         intrinsic_debug_log::block_data block_data;

         // Read all data for the current block.
         {
            uint8_t tag = 0;
            log.read( (char*)&tag, sizeof(tag) );
            FC_ASSERT( tag == block_tag, "corrupted log: expected block tag" );
            log.read( (char*)&block_data.block_num, sizeof(block_data.block_num) );

            bool in_transaction = false;
            bool in_action      = false;
            uint64_t minimum_accepted_global_sequence_num = 0;

            for(;;) {
               log.read( (char*)&tag, sizeof(tag) );
               if( tag == block_tag ) {
                  decltype(last_commited_block_pos) read_pos{};
                  log.read( (char*)&read_pos, sizeof(read_pos) );
                  FC_ASSERT( read_pos == pos, "corrupted log: block position did not match" );
                  break;
               } else if( tag == transaction_tag ) {
                  char buffer[32];
                  log.read( buffer, sizeof(buffer) );
                  fc::datastream<char*> ds( buffer, sizeof(buffer) );
                  block_data.transactions.emplace_back();
                  fc::raw::unpack( ds, block_data.transactions.back().trx_id );
                  in_transaction = true;
                  in_action      = false;
               } else if( tag == action_tag ) {
                  FC_ASSERT( in_transaction, "corrupted log: no start of transaction before encountering action tag");
                  uint64_t global_sequence_num = 0;
                  log.read( (char*)&global_sequence_num, sizeof(global_sequence_num) );
                  FC_ASSERT( global_sequence_num >= minimum_accepted_global_sequence_num,
                             "corrupted log: global sequence numbers of actions within a block are not monotonically increasing"
                  );
                  block_data.transactions.back().actions.emplace_back();
                  auto& act = block_data.transactions.back().actions.back();
                  act.global_sequence_num = global_sequence_num;
                  log.read( (char*)&act.receiver.value,        sizeof(act.receiver.value) );
                  log.read( (char*)&act.first_receiver.value,  sizeof(act.first_receiver.value) );
                  log.read( (char*)&act.action_name.value,     sizeof(act.action_name.value) );
                  in_action = true;
                  minimum_accepted_global_sequence_num = global_sequence_num + 1;
               } else if( tag == intrinsic_tag ) {
                  FC_ASSERT( in_action, "corrupted log: no start of action before encountering intrinsic tag");
                  auto& intrinsics = block_data.transactions.back().actions.back().recorded_intrinsics;
                  uint32_t minimum_accepted_ordinal = 0;
                  if( intrinsics.size() > 0 ) {
                     minimum_accepted_ordinal = intrinsics.back().intrinsic_ordinal + 1;
                  }
                  uint32_t ordinal = 0;
                  log.read( (char*)&ordinal, sizeof(ordinal) );
                  FC_ASSERT( ordinal >= minimum_accepted_ordinal,
                             "corrupted log: intrinsic ordinals within an action are not monotonically increasing" );
                  char buffer[64];
                  log.read( buffer, sizeof(buffer) );
                  fc::datastream<char*> ds( buffer, sizeof(buffer) );
                  intrinsics.emplace_back();
                  auto& intrinsic = intrinsics.back();
                  intrinsic.intrinsic_ordinal = ordinal;
                  fc::raw::unpack( ds, intrinsic.arguments_hash );
                  fc::raw::unpack( ds, intrinsic.memory_hash );
               } else {
                  FC_ASSERT( false, "corrupted log: unrecognized tag" );
               }
            }

            FC_ASSERT( block_data.transactions.size() > 0, "corrupted log: empty block included" );
         }

         int64_t next_pos = -1;
         if( log.tellg() < file_size ) next_pos = log.tellg();

         auto res = block_data_cache.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple( static_cast<uint64_t>(pos) ),
                        std::forward_as_tuple( std::move(block_data), pos, previous_pos, next_pos )
         );
         return res.first->second;
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
      fc::raw::pack( ds, trx_id );
      FC_ASSERT( !ds.remaining(), "unexpected size for transaction id" );

      my->ensure_block_start_written();
      my->log.put( detail::intrinsic_debug_log_impl::transaction_tag );
      my->log.write( buffer, sizeof(buffer) );

      my->state = detail::intrinsic_debug_log_impl::state_t::in_transaction;
      my->intrinsic_counter = 0;
   }

   void intrinsic_debug_log::start_action( uint64_t global_sequence_num,
                                           name receiver, name first_receiver, name action_name )
   {
      FC_ASSERT( my->state != detail::intrinsic_debug_log_impl::state_t::read_only,
                 "invalid operation in read-only mode" );
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::in_transaction
                 || my->state == detail::intrinsic_debug_log_impl::state_t::in_action,
                 "cannot start action in the current state" );

      my->log.put( detail::intrinsic_debug_log_impl::action_tag );
      my->log.write( (char*)&global_sequence_num,  sizeof(global_sequence_num) );
      my->log.write( (char*)&receiver.value,       sizeof(receiver.value) );
      my->log.write( (char*)&first_receiver.value, sizeof(first_receiver.value) );
      my->log.write( (char*)&action_name.value,    sizeof(action_name.value) );

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
      fc::raw::pack( ds, arguments_hash );
      fc::raw::pack( ds, memory_hash );
      FC_ASSERT( !ds.remaining(), "unexpected size for digest" );

      my->log.put( detail::intrinsic_debug_log_impl::intrinsic_tag );
      my->log.write( (char*)&my->intrinsic_counter, sizeof(my->intrinsic_counter) );
      my->log.write( buffer, sizeof(buffer) );

      ++(my->intrinsic_counter);
   }

   intrinsic_debug_log::block_iterator intrinsic_debug_log::begin_block()const {
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::read_only,
                 "block_begin is only allowed in read-only mode" );
      return block_iterator( my.get(), (my->last_commited_block_pos == 0ull ? -1ll : 0ll) );
   }

   intrinsic_debug_log::block_iterator intrinsic_debug_log::end_block()const {
      FC_ASSERT( my->state == detail::intrinsic_debug_log_impl::state_t::read_only,
                 "block_end is only allowed in read-only mode" );
      return block_iterator( my.get() );
   }

   const intrinsic_debug_log::block_data* intrinsic_debug_log::block_iterator::get_pointer()const {
      FC_ASSERT( _log, "cannot dereference default constructed block iterator" );
      FC_ASSERT( _pos >= 0, "cannot dereference an end block iterator" );

      if( !_cached_block_data ) {
         _cached_block_data = &_log->get_block_at_position( _pos );
      }

      return &(_cached_block_data->block_data);

   }

   intrinsic_debug_log::block_iterator& intrinsic_debug_log::block_iterator::operator++() {
      FC_ASSERT( _log, "cannot increment default constructed block iterator" );
      FC_ASSERT( _pos >= 0, "cannot increment end block iterator" );

      if( !_cached_block_data ) {
         _cached_block_data = &_log->get_block_at_position( _pos );
      }

      _pos = _cached_block_data->next_pos;
      _cached_block_data = nullptr;

      return *this;
   }

   intrinsic_debug_log::block_iterator& intrinsic_debug_log::block_iterator::operator--() {
      FC_ASSERT( _log, "cannot decrement default constructed block iterator" );

      if( _pos < 0 ) {
         FC_ASSERT( !_cached_block_data, "invariant failure" );
         FC_ASSERT( _log->file_size > 0, "cannot decrement end block iterator when the log contains no blocks" );
         _pos = static_cast<int64_t>( _log->last_commited_block_pos );
         return *this;
      }

      int64_t prev_pos = -1;
      if( _cached_block_data ) {
         prev_pos = _cached_block_data->previous_pos;
      } else {
         prev_pos = _log->get_position_of_previous_block( _pos );
      }

      FC_ASSERT( prev_pos >= 0, "cannot decrement begin block iterator" );
      _pos = prev_pos;
      _cached_block_data = nullptr;
      return *this;
   }

   std::optional<intrinsic_debug_log::intrinsic_differences>
   intrinsic_debug_log::find_first_difference( intrinsic_debug_log& lhs, intrinsic_debug_log& rhs ) {
      lhs.close();
      rhs.close();
      lhs.open( intrinsic_debug_log::open_mode::read_only );
      rhs.open( intrinsic_debug_log::open_mode::read_only );

      auto close_logs = fc::make_scoped_exit([&lhs, &rhs] {
         lhs.close();
         rhs.close();
      });

      enum class mismatch_t {
         equivalent,
         lhs_ended_early,
         rhs_ended_early,
         mismatch
      };

      auto find_if_mismatched = []( auto&& lhs_begin, auto&& lhs_end,
                                    auto&& rhs_begin, auto&& rhs_end,
                                    auto&& compare_equivalent ) -> mismatch_t
      {
         for( ; lhs_begin != lhs_end && rhs_begin != rhs_end; ++lhs_begin, ++rhs_begin ) {
            const auto& lhs_value = *lhs_begin;
            const auto& rhs_value = *rhs_begin;
            if( compare_equivalent( lhs_value, rhs_value ) ) continue;
            return mismatch_t::mismatch;
         }

         bool lhs_empty = (lhs_begin == lhs_end);
         bool rhs_empty = (rhs_begin == rhs_end);
         // lhs_empty and rhs_empty cannot both be false at this point since we exited the for loop.

         if( lhs_empty && !rhs_empty ) return mismatch_t::lhs_ended_early;
         if( !lhs_empty && rhs_empty ) return mismatch_t::rhs_ended_early;
         return mismatch_t::equivalent;
      };

      std::optional<intrinsic_differences> result;
      const char* error_msg = nullptr;

      auto status = find_if_mismatched(
         lhs.begin_block(), rhs.end_block(),
         rhs.begin_block(), rhs.end_block(),
         [&find_if_mismatched, &result, &error_msg]
         ( const block_data& block_lhs, const block_data& block_rhs ) {
            if( block_lhs.block_num != block_rhs.block_num ) {
               error_msg = "logs do not have the same blocks";
               return false;
            }
            auto status2 = find_if_mismatched(
               block_lhs.transactions.begin(), block_lhs.transactions.end(),
               block_rhs.transactions.begin(), block_rhs.transactions.end(),
               [&find_if_mismatched, &result, &error_msg, &block_lhs]
               ( const transaction_data& trx_lhs, const transaction_data& trx_rhs ) {
                  if( trx_lhs.trx_id != trx_rhs.trx_id ) {
                     return false;
                  }
                  auto status3 = find_if_mismatched(
                     trx_lhs.actions.begin(), trx_lhs.actions.end(),
                     trx_rhs.actions.begin(), trx_rhs.actions.end(),
                     [&result, &block_lhs, &trx_lhs]
                     ( const action_data& act_lhs, const action_data& act_rhs ) {
                        if( std::tie( act_lhs.global_sequence_num,
                                      act_lhs.receiver,
                                      act_lhs.first_receiver,
                                      act_lhs.action_name )
                              != std::tie( act_rhs.global_sequence_num,
                                           act_rhs.receiver,
                                           act_rhs.first_receiver,
                                           act_rhs.action_name )
                        ) {
                           return false;
                        }
                        if( act_lhs.recorded_intrinsics != act_rhs.recorded_intrinsics ) {
                           result.emplace( intrinsic_differences{
                              .block_num               = block_lhs.block_num,
                              .trx_id                  = trx_lhs.trx_id,
                              .global_sequence_num     = act_lhs.global_sequence_num,
                              .receiver                = act_lhs.receiver,
                              .first_receiver          = act_lhs.first_receiver,
                              .action_name             = act_lhs.action_name,
                              .lhs_recorded_intrinsics = act_lhs.recorded_intrinsics,
                              .rhs_recorded_intrinsics = act_rhs.recorded_intrinsics
                           } );
                           return false;
                        }
                        return true;
                     }
                  );
                  if( status3 == mismatch_t::equivalent ) return true;
                  if( error_msg == nullptr && !result ) {
                     error_msg = "different actions within a particular transaction";
                  }
                  return false;
               }
            );
            if( status2 == mismatch_t::equivalent ) return true;
            if( error_msg == nullptr && !result ) {
               error_msg = "different transactions in a particular block";
            }
            return false;
         }
      );

      if( status == mismatch_t::mismatch ) {
         if( error_msg != nullptr ) {
            FC_THROW_EXCEPTION( fc::exception, error_msg );
         }
         return result;
      }

      FC_ASSERT( status == mismatch_t::equivalent, "logs do not have the same blocks" );
      return {}; // the logs are equivalent
   }

} } /// eosio::chain
