/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace boost {
   namespace filesystem {
      class path;
   }
}

namespace eosio { namespace chain {

   namespace detail { class intrinsic_debug_log_impl; }

   class intrinsic_debug_log {
      public:
         intrinsic_debug_log( const boost::filesystem::path& log_path );
         intrinsic_debug_log( intrinsic_debug_log&& other );
         ~intrinsic_debug_log();

         enum class open_mode {
            read_only,
            create_new,
            continue_existing
         };

         void open( open_mode mode = open_mode::continue_existing );
         void close();
         void flush();

         void start_block( uint32_t block_num );
         void start_transaction( const transaction_id_type& trx_id );
         void start_action( uint64_t global_sequence_num );
         void acknowledge_intrinsic_without_recording();
         void record_intrinsic( const digest_type& arguments_hash, const digest_type& memory_hash );
         void finish_block();

      private:
         std::unique_ptr<detail::intrinsic_debug_log_impl> my;
   };

} }
