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
#include <variant>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using pos_t = uint64_t;

namespace eosio { namespace chain {

   namespace detail {

      template<typename T, typename... Us>
      struct is_one_of {
         static constexpr bool value = (std::is_same_v<T, Us> || ...);
      };

      struct extended_block_data {
         extended_block_data( intrinsic_debug_log::block_data&& block_data,
                              const pos_t& pos,
                              const std::optional<pos_t>& previous_pos,
                              const std::optional<pos_t>& next_pos )
         :block_data( std::move(block_data) )
         ,pos( pos )
         ,previous_pos( previous_pos )
         ,next_pos( next_pos )
         {}

         intrinsic_debug_log::block_data block_data;
         pos_t                           pos;
         std::optional<pos_t>            previous_pos; //< if empty, then this is first block in the log
         std::optional<pos_t>            next_pos; //< if empty, then this is the last block in the log
      };

      class intrinsic_debug_log_impl {
         public:
            static constexpr uint8_t block_tag       = 0;
            static constexpr uint8_t transaction_tag = 1;
            static constexpr uint8_t action_tag      = 2;
            static constexpr uint8_t intrinsic_tag   = 3;

            struct closed {};

            struct read_only {
               explicit read_only( uint32_t last_committed_block_num )
               :last_committed_block_num( last_committed_block_num )
               {}

               uint32_t last_committed_block_num;
            };

            struct in_block;
            struct in_transaction;
            struct in_action;

            struct waiting_to_start_block {
               explicit waiting_to_start_block( uint32_t last_committed_block_num );
               explicit waiting_to_start_block( const in_block& s );
               explicit waiting_to_start_block( const in_transaction& s );
               explicit waiting_to_start_block( const in_action& s );

               uint32_t last_committed_block_num;
            };

            struct in_block {
               in_block( const waiting_to_start_block& s, pos_t pos, uint32_t num );
               explicit in_block( const in_transaction& s );
               explicit in_block( const in_action& s );

               pos_t    block_start_pos;
               uint32_t block_num;
               uint32_t last_committed_block_num;
               bool     block_header_written = false;
            };

            struct in_transaction {
               in_transaction( const in_block& s, pos_t transaction_start_pos );
               in_transaction( const in_transaction& s, pos_t transaction_start_pos );
               in_transaction( const in_action& s, pos_t transaction_start_pos );

               pos_t    block_start_pos;
               pos_t    transaction_start_pos;
               uint32_t block_num;
               uint32_t last_committed_block_num;
            };

            struct in_action {
               struct force_new_t {};

               explicit in_action( const in_transaction& s );
               in_action( const in_transaction& s, force_new_t ):in_action( s ){}
               in_action( const in_action& s, force_new_t );

               pos_t    block_start_pos;
               pos_t    transaction_start_pos;
               uint32_t block_num;
               uint32_t last_committed_block_num;
               uint32_t intrinsic_counter = 0;
            };

            using state_type = std::variant<
                                 closed,
                                 read_only,
                                 waiting_to_start_block,
                                 in_block,
                                 in_transaction,
                                 in_action
                               >;

            struct log_metadata_t {
               pos_t    last_committed_block_begin_pos{};
               pos_t    last_committed_block_end_pos{};
            };

            std::fstream                          log;
            bfs::path                             log_path;
            std::map<pos_t, extended_block_data>  block_data_cache;
            state_type                            state{ closed{} };
            std::optional<log_metadata_t>         log_metadata;
            bool                                  finish_block_on_shutdown = false;

            void open( bool open_as_read_only, bool start_fresh, bool auto_finish_block );
            void close();

            void start_block( uint32_t block_num );
            void abort_block();
            void start_transaction( const transaction_id_type& trx_id );
            void abort_transaction();
            void start_action( uint64_t global_sequence_num,
                               name receiver, name first_receiver, name action_name );
            void acknowledge_intrinsic_without_recording();
            void record_intrinsic( const digest_type& arguments_hash, const digest_type& memory_hash );
            void finish_block();

            bool is_open()const {
               return !std::holds_alternative<closed>( state );
            }

            bool is_read_only()const {
               return std::holds_alternative<read_only>( state );
            }

            bool can_write()const {
               return !std::holds_alternative<closed>( state ) && !std::holds_alternative<read_only>( state );
            }

         protected:
            void truncate_to( uint64_t pos );
            std::optional<pos_t> get_position_of_previous_block( pos_t pos );
            const extended_block_data& get_block_at_position( pos_t pos );

            friend class intrinsic_debug_log::block_iterator;
      };

      intrinsic_debug_log_impl::waiting_to_start_block::waiting_to_start_block( uint32_t last_committed_block_num )
      :last_committed_block_num( last_committed_block_num )
      {}

      intrinsic_debug_log_impl::waiting_to_start_block::waiting_to_start_block( const in_block& s )
      :last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::waiting_to_start_block::waiting_to_start_block( const in_transaction& s )
      :last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::waiting_to_start_block::waiting_to_start_block( const in_action& s )
      :last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_block::in_block( const waiting_to_start_block& s, pos_t pos, uint32_t num )
      :block_start_pos( pos )
      ,block_num( num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_block::in_block( const in_transaction& s )
      :block_start_pos( s.block_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      ,block_header_written(true)
      {}

      intrinsic_debug_log_impl::in_block::in_block( const in_action& s )
      :block_start_pos( s.block_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      ,block_header_written(true)
      {}

      intrinsic_debug_log_impl::in_transaction::in_transaction( const in_block& s, pos_t transaction_start_pos )
      :block_start_pos( s.block_start_pos )
      ,transaction_start_pos( transaction_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_transaction::in_transaction( const in_transaction& s, pos_t transaction_start_pos )
      :block_start_pos( s.block_start_pos )
      ,transaction_start_pos( transaction_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_transaction::in_transaction( const in_action& s, pos_t transaction_start_pos )
      :block_start_pos( s.block_start_pos )
      ,transaction_start_pos( transaction_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_action::in_action( const in_transaction& s )
      :block_start_pos( s.block_start_pos )
      ,transaction_start_pos( s.transaction_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      intrinsic_debug_log_impl::in_action::in_action( const in_action& s, in_action::force_new_t )
      :block_start_pos( s.block_start_pos )
      ,transaction_start_pos( s.transaction_start_pos )
      ,block_num( s.block_num )
      ,last_committed_block_num( s.last_committed_block_num )
      {}

      void intrinsic_debug_log_impl::open( bool open_as_read_only, bool start_fresh, bool auto_finish_block ) {
         FC_ASSERT( !log.is_open(), "cannot open when the log is already open" );
         FC_ASSERT( std::holds_alternative<closed>( state ), "state out of sync" );

         std::ios_base::openmode open_mode = std::ios::in | std::ios::binary;

         if( open_as_read_only ) {
            FC_ASSERT( !start_fresh,       "start_fresh cannot be true in read-only mode" );
            FC_ASSERT( !auto_finish_block, "auto_finish_block cannot be true in read-only mode" );
         } else {
            open_mode |= (std::ios::out | std::ios::app);
         }

         if( start_fresh ) {
            open_mode |= std::ios::trunc;
         }

         log.open( log_path.generic_string().c_str(), open_mode );

         uint32_t last_committed_block_num = 0; // 0 indicates no committed blocks in log

         try {
            log.seekg( 0, std::ios::end );
            pos_t file_size = log.tellg();
            if( file_size > 0 ) {
               pos_t last_committed_block_begin_pos{};
               FC_ASSERT( file_size > sizeof(last_committed_block_begin_pos),
                          "corrupted log: file size is too small to be valid" );
               log.seekg( -sizeof(last_committed_block_begin_pos), std::ios::end );
               fc::raw::unpack( log, last_committed_block_begin_pos );
               FC_ASSERT( last_committed_block_begin_pos < file_size, "corrupted log: invalid block position" );

               log.seekg( last_committed_block_begin_pos, std::ios::beg	);
               uint8_t tag = 0;
               fc::raw::unpack( log, tag );
               FC_ASSERT( tag == block_tag, "corrupted log: expected block tag" );

               fc::raw::unpack( log, last_committed_block_num );

               log_metadata.emplace( log_metadata_t{ last_committed_block_begin_pos, file_size } );
            }
         } catch( ... ) {
            log.close();
            throw;
         }

         finish_block_on_shutdown = auto_finish_block;

         if( open_as_read_only ) {
            state.emplace<read_only>( last_committed_block_num );
            log.seekg( 0, std::ios::beg );
         } else {
            state.emplace<waiting_to_start_block>( last_committed_block_num );
            log.seekp( 0, std::ios::end );
         }
      }

      void intrinsic_debug_log_impl::close() {
         if( !log.is_open() ) return;

         if( finish_block_on_shutdown
               && !std::holds_alternative<closed>( state )
               && !std::holds_alternative<read_only>( state )
               && !std::holds_alternative<waiting_to_start_block>( state )
         ) {
            finish_block();
         }

         std::optional<pos_t> truncate_pos = std::visit( overloaded{
            []( closed ) -> std::optional<pos_t> {
               FC_ASSERT( false, "state out of sync" );
               return {}; // unreachable, only added for compilation purposes
            },
            []( const read_only& ) -> std::optional<pos_t> { return {}; },
            []( const waiting_to_start_block&  ) -> std::optional<pos_t> { return {}; },
            []( const in_block&  ) -> std::optional<pos_t> { return {}; },
            []( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_transaction, in_action
                  >::value,
                  std::optional<pos_t>
            >
            {
               return s.block_start_pos;
            }
         }, state );

         if( truncate_pos ) {
            truncate_to( *truncate_pos );
         } else {
            log.flush();
         }

         log.close();

         finish_block_on_shutdown = false;

         state.emplace<closed>();
         log_metadata.reset();
         block_data_cache.clear();
      }

      void intrinsic_debug_log_impl::truncate_to( pos_t pos ) {
         FC_ASSERT( can_write(),  "called while in invalid state" );

         log.seekp( 0, std::ios::end );
         pos_t file_size = log.tellp();
         FC_ASSERT( 0 <= pos && pos <= file_size, "position is out of bounds" );

         FC_ASSERT( !log_metadata || log_metadata->last_committed_block_end_pos <= pos,
                    "truncation would erase committed block data which is currently not supported" );

         log.flush();
         if( pos == file_size ) return;

         if( log.tellp() > pos ) {
            log.seekp( pos );
         }

         bfs::resize_file( log_path, pos );

         log.sync();
      }

      void intrinsic_debug_log_impl::start_block( uint32_t block_num  ) {
         FC_ASSERT( block_num > 0, "block number cannot be 0" );
         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            [&]( const waiting_to_start_block& s ) {
               FC_ASSERT( s.last_committed_block_num < block_num,
                          "must start a block with greater height than previously started blocks" );
               state = in_block( s, log.tellp(), block_num );
               return true;
            },
            []( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_block, in_transaction, in_action
                  >::value,
                  bool
            >
            {
               FC_ASSERT( false, "cannot start block while still processing a block" );
               return false; // unreachable, only added for compilation purposes
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      void intrinsic_debug_log_impl::abort_block() {
         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            []( const waiting_to_start_block& ) {
               return true; // nothing to abort
            },
            [&]( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_block, in_transaction, in_action
                  >::value,
                  bool
            >
            {
               truncate_to( s.block_start_pos );
               state = waiting_to_start_block( s );
               return true;
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      void intrinsic_debug_log_impl::start_transaction( const transaction_id_type& trx_id ) {
         auto write_transaction_header = [&] {
            uint64_t transaction_start_pos = log.tellp();
            fc::raw::pack( log, transaction_tag );
            fc::raw::pack( log, trx_id );
            return transaction_start_pos;
         };

         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            []( const waiting_to_start_block& ) {
               FC_ASSERT( false, "cannot start transaction in the current state" );
               return false; // unreachable, only added for compilation purposes
            },
            [&]( const in_block& s ) {
               if( !s.block_header_written ) {
                  fc::raw::pack( log, block_tag );
                  fc::raw::pack( log, s.block_num );
               }
               uint64_t transaction_start_pos = write_transaction_header();
               state = in_transaction( s, transaction_start_pos );
               return true;
            },
            [&]( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_transaction, in_action
                  >::value,
                  bool
            >
            {
               uint64_t transaction_start_pos = write_transaction_header();
               state = in_transaction( s, transaction_start_pos );
               return true;
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      void intrinsic_debug_log_impl::abort_transaction() {
         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            []( const waiting_to_start_block& ) {
               FC_ASSERT( false, "cannot abort transaction in the current state" );
               return false; // unreachable, only added for compilation purposes
            },
            []( const in_block& s ) {
               FC_ASSERT( false, "cannot abort transaction in the current state" );
               return false; // unreachable, only added for compilation purposes
            },
            [&]( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_transaction, in_action
                  >::value,
                  bool
            >
            {
               truncate_to( s.transaction_start_pos );
               state = in_block( s );
               return true;
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      void intrinsic_debug_log_impl::start_action( uint64_t global_sequence_num,
                                                   name receiver, name first_receiver, name action_name )
      {
         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            []( const waiting_to_start_block& ) {
               FC_ASSERT( false, "cannot start action in the current state" );
               return false; // unreachable, only added for compilation purposes
            },
            []( const in_block& ) {
               FC_ASSERT( false, "cannot start action in the current state" );
               return false; // unreachable, only added for compilation purposes
            },
            [&]( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_transaction, in_action
                  >::value,
                  bool
            >
            {
               fc::raw::pack( log, action_tag );
               fc::raw::pack( log, global_sequence_num );
               fc::raw::pack( log, receiver );
               fc::raw::pack( log, first_receiver );
               fc::raw::pack( log, action_name );
               state = in_action( s, in_action::force_new_t{} );
               return true;
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      void intrinsic_debug_log_impl::acknowledge_intrinsic_without_recording() {
         FC_ASSERT( can_write(), "log is not setup for writing" );
         in_action& s = std::get<in_action>( state );
         ++(s.intrinsic_counter);
      }

      void intrinsic_debug_log_impl::record_intrinsic( const digest_type& arguments_hash, const digest_type& memory_hash ) {
         FC_ASSERT( can_write(), "log is not setup for writing" );
         in_action& s = std::get<in_action>( state );

         fc::raw::pack( log, intrinsic_tag );
         fc::raw::pack( log, s.intrinsic_counter );
         fc::raw::pack( log, arguments_hash );
         fc::raw::pack( log, memory_hash );

         ++(s.intrinsic_counter);
      }

      void intrinsic_debug_log_impl::finish_block() {
         bool writable = std::visit( overloaded{
            []( closed ) { return false; },
            []( const read_only& ) { return false; },
            []( const waiting_to_start_block& ) {
               FC_ASSERT( false, "no started block" );
               return false; // unreachable, only added for compilation purposes
            },
            [&]( const in_block& s ) {
               state = waiting_to_start_block( s.last_committed_block_num );
               return true; // nothing to commit
            },
            [&]( auto&& s )
            -> std::enable_if_t<
                  detail::is_one_of< std::decay_t<decltype(s)>,
                     in_transaction, in_action
                  >::value,
                  bool
            >
            {
               fc::raw::pack( log, block_tag );
               fc::raw::pack( log, s.block_start_pos );
               pos_t new_last_committed_block_end_pos = log.tellp();
               log.flush();

               pos_t new_last_committed_block_begin_pos{};
               if( log_metadata ) {
                  new_last_committed_block_begin_pos = log_metadata->last_committed_block_end_pos;
               }

               log_metadata.emplace( log_metadata_t{ new_last_committed_block_begin_pos, new_last_committed_block_end_pos} );

               state = waiting_to_start_block( s.last_committed_block_num );
               return true;
            }
         }, state );
         FC_ASSERT( writable, "log is not setup for writing" );
      }

      std::optional<pos_t> intrinsic_debug_log_impl::get_position_of_previous_block( pos_t pos ) {
         std::optional<pos_t> previous_pos;
         if( pos > sizeof(pos_t) ) {
            pos_t read_pos = 0;
            log.seekg( pos - sizeof(read_pos), std::ios::beg );
            fc::raw::unpack( log, read_pos );
            FC_ASSERT( read_pos < log.tellg(), "corrupted log: invalid block position" );
            previous_pos = read_pos;
         }
         return previous_pos;
      }

      const extended_block_data& intrinsic_debug_log_impl::get_block_at_position( pos_t pos ) {
         FC_ASSERT( is_read_only(), "only allowed in read-only mode" );
         FC_ASSERT( log_metadata && 0 <= pos && pos < log_metadata->last_committed_block_end_pos,
                    "position is out of bounds" );

         auto itr = block_data_cache.find( pos );
         if( itr != block_data_cache.end() ) return itr->second;

         std::optional<pos_t> previous_pos = get_position_of_previous_block( pos );
         log.seekg( pos, std::ios::beg );

         intrinsic_debug_log::block_data block_data;

         // Read all data for the current block.
         {
            uint8_t tag = 0;
            fc::raw::unpack( log, tag );
            FC_ASSERT( tag == block_tag, "corrupted log: expected block tag" );
            fc::raw::unpack( log, block_data.block_num );

            bool in_transaction = false;
            bool in_action      = false;
            uint64_t minimum_accepted_global_sequence_num = 0;

            for(;;) {
               fc::raw::unpack( log, tag );
               if( tag == block_tag ) {
                  pos_t read_pos{};
                  fc::raw::unpack( log, read_pos );
                  FC_ASSERT( read_pos == pos, "corrupted log: block position did not match" );
                  break;
               } else if( tag == transaction_tag ) {
                  block_data.transactions.emplace_back();
                  fc::raw::unpack( log, block_data.transactions.back().trx_id );
                  in_transaction = true;
                  in_action      = false;
               } else if( tag == action_tag ) {
                  FC_ASSERT( in_transaction, "corrupted log: no start of transaction before encountering action tag");
                  uint64_t global_sequence_num = 0;
                  fc::raw::unpack( log, global_sequence_num );
                  FC_ASSERT( global_sequence_num >= minimum_accepted_global_sequence_num,
                             "corrupted log: global sequence numbers of actions within a block are not monotonically increasing"
                  );
                  block_data.transactions.back().actions.emplace_back();
                  auto& act = block_data.transactions.back().actions.back();
                  act.global_sequence_num = global_sequence_num;
                  fc::raw::unpack( log, act.receiver );
                  fc::raw::unpack( log, act.first_receiver );
                  fc::raw::unpack( log, act.action_name );
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
                  fc::raw::unpack( log, ordinal );
                  FC_ASSERT( ordinal >= minimum_accepted_ordinal,
                             "corrupted log: intrinsic ordinals within an action are not monotonically increasing" );
                  intrinsics.emplace_back();
                  auto& intrinsic = intrinsics.back();
                  intrinsic.intrinsic_ordinal = ordinal;
                  fc::raw::unpack( log, intrinsic.arguments_hash );
                  fc::raw::unpack( log, intrinsic.memory_hash );
               } else {
                  FC_ASSERT( false, "corrupted log: unrecognized tag" );
               }
            }

            FC_ASSERT( block_data.transactions.size() > 0, "corrupted log: empty block included" );
         }

         std::optional<pos_t> next_pos;
         if( log.tellg() < log_metadata->last_committed_block_end_pos ) next_pos = log.tellg();

         auto res = block_data_cache.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple( pos ),
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
         try {
            my->close();
         } FC_LOG_AND_DROP()
         my.reset();
      }
   }

   void intrinsic_debug_log::open( open_mode mode ) {
      bool open_as_read_only = (mode == open_mode::read_only);
      bool start_fresh = false;
      bool auto_finish_block = false;

      if( !open_as_read_only ) {
         start_fresh = ( mode == open_mode::create_new || mode == open_mode::create_new_and_auto_finish_block );
         auto_finish_block = ( mode == open_mode::create_new_and_auto_finish_block
                                 || mode == open_mode::continue_existing_and_auto_finish_block );
      }

      my->open( open_as_read_only, start_fresh, auto_finish_block );
   }

   void intrinsic_debug_log::close() {
      my->close();
   }

   void intrinsic_debug_log::flush() {
      my->log.flush();
   }

   void intrinsic_debug_log::start_block( uint32_t block_num ) {
      my->start_block( block_num );
   }

   void intrinsic_debug_log::abort_block() {
      my->abort_block();
   }

   void intrinsic_debug_log::start_transaction( const transaction_id_type& trx_id ) {
      my->start_transaction( trx_id );
   }

   void intrinsic_debug_log::abort_transaction() {
      my->abort_transaction();
   }

   void intrinsic_debug_log::start_action( uint64_t global_sequence_num,
                                           name receiver, name first_receiver, name action_name )
   {
      my->start_action( global_sequence_num, receiver, first_receiver, action_name );
   }

   void intrinsic_debug_log::acknowledge_intrinsic_without_recording() {
      my->acknowledge_intrinsic_without_recording();
   }

   void intrinsic_debug_log::record_intrinsic( const digest_type& arguments_hash, const digest_type& memory_hash ) {
      my->record_intrinsic( arguments_hash, memory_hash );
   }

   void intrinsic_debug_log::finish_block() {
      my->finish_block();
   }

   bool intrinsic_debug_log::is_open()const {
      return my->is_open();
   }

   bool intrinsic_debug_log::is_read_only()const {
      return my->is_read_only();
   }

   const boost::filesystem::path& intrinsic_debug_log::get_path()const {
      return my->log_path;
   }

   uint32_t intrinsic_debug_log::last_committed_block_num()const {
      std::optional<uint32_t> result = std::visit( overloaded{
         []( detail::intrinsic_debug_log_impl::closed ) -> std::optional<uint32_t> { return {}; },
         [&]( auto&& s )
         -> std::enable_if_t<
               detail::is_one_of< std::decay_t<decltype(s)>,
                  detail::intrinsic_debug_log_impl::read_only,
                  detail::intrinsic_debug_log_impl::waiting_to_start_block,
                  detail::intrinsic_debug_log_impl::in_block,
                  detail::intrinsic_debug_log_impl::in_transaction,
                  detail::intrinsic_debug_log_impl::in_action
               >::value,
               std::optional<uint32_t>
         >
         {
            return s.last_committed_block_num;
         }
      }, my->state );
      FC_ASSERT( result, "log is closed" );
      return *result;
   }

   intrinsic_debug_log::block_iterator intrinsic_debug_log::begin_block()const {
      FC_ASSERT( my->is_read_only(), "block_begin is only allowed in read-only mode" );
      return block_iterator( my.get(), (my->log_metadata ? 0ll : -1ll) );
   }

   intrinsic_debug_log::block_iterator intrinsic_debug_log::end_block()const {
      FC_ASSERT( my->is_read_only(), "block_end is only allowed in read-only mode" );
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
         _cached_block_data = &_log->get_block_at_position( static_cast<pos_t>(_pos) );
      }

      _pos = (_cached_block_data->next_pos ? static_cast<int64_t>(*_cached_block_data->next_pos) : -1ll);
      _cached_block_data = nullptr;

      return *this;
   }

   intrinsic_debug_log::block_iterator& intrinsic_debug_log::block_iterator::operator--() {
      FC_ASSERT( _log, "cannot decrement default constructed block iterator" );

      if( _pos < 0 ) {
         FC_ASSERT( !_cached_block_data, "invariant failure" );
         FC_ASSERT( _log->log_metadata, "cannot decrement end block iterator when the log contains no blocks" );
         _pos = static_cast<int64_t>( _log->log_metadata->last_committed_block_begin_pos );
         return *this;
      }

      std::optional<pos_t> prev_pos;
      if( _cached_block_data ) {
         prev_pos = _cached_block_data->previous_pos;
      } else {
         prev_pos = _log->get_position_of_previous_block( static_cast<pos_t>(_pos) );
      }

      FC_ASSERT( prev_pos, "cannot decrement begin block iterator" );
      _pos = static_cast<int64_t>(*prev_pos);
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
         lhs.begin_block(), lhs.end_block(),
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
