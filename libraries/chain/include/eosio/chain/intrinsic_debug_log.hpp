/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <iterator>

namespace boost {
   namespace filesystem {
      class path;
   }
}

namespace eosio { namespace chain {

   namespace detail {
      struct extended_block_data;
      class intrinsic_debug_log_impl;
   }

   class intrinsic_debug_log {
      public:
         intrinsic_debug_log( const boost::filesystem::path& log_path );
         intrinsic_debug_log( intrinsic_debug_log&& other );
         ~intrinsic_debug_log();

         enum class open_mode {
            read_only,
            create_new,
            continue_existing,
            create_new_and_auto_finish_block,
            continue_existing_and_auto_finish_block
         };

         void open( open_mode mode = open_mode::continue_existing );
         void flush();

         /** invalidates all block_iterators */
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

         struct intrinsic_record {
            uint32_t                        intrinsic_ordinal = 0;
            digest_type                     arguments_hash;
            digest_type                     memory_hash;

            friend bool operator == ( const intrinsic_record& lhs, const intrinsic_record& rhs ) {
               return std::tie( lhs.intrinsic_ordinal, lhs.arguments_hash, lhs.memory_hash )
                        == std::tie( rhs.intrinsic_ordinal, rhs.arguments_hash, rhs.memory_hash );
            }

            friend bool operator != ( const intrinsic_record& lhs, const intrinsic_record& rhs ) {
               return !(lhs == rhs);
            }
         };

         struct action_data {
            uint64_t                        global_sequence_num = 0;
            name                            receiver{};
            name                            first_receiver{};
            name                            action_name{};
            std::vector<intrinsic_record>   recorded_intrinsics;
         };

         struct transaction_data {
            transaction_id_type             trx_id;
            std::vector< action_data >      actions;
         };

         struct block_data {
            uint32_t                        block_num = 0;
            std::vector<transaction_data>   transactions;
         };

         class block_iterator : public std::iterator<std::bidirectional_iterator_tag, const block_data> {
         protected:
            detail::intrinsic_debug_log_impl*           _log = nullptr;
            int64_t                                     _pos = -1;
            mutable const detail::extended_block_data*  _cached_block_data = nullptr;

         protected:
            explicit block_iterator( detail::intrinsic_debug_log_impl* log, int64_t pos = -1 )
            :_log(log)
            ,_pos(pos)
            {}

            const block_data* get_pointer()const;

            friend class intrinsic_debug_log;

         public:
            block_iterator() = default;

            friend bool operator == ( const block_iterator& lhs, const block_iterator& rhs ) {
               return std::tie( lhs._log, lhs._pos ) == std::tie( rhs._log, rhs._pos );
            }

            friend bool operator != ( const block_iterator& lhs, const block_iterator& rhs ) {
               return !(lhs == rhs);
            }

            const block_data& operator*()const {
               return *get_pointer();
            }

            const block_data* operator->()const {
               return get_pointer();
            }

            block_iterator& operator++();

            block_iterator& operator--();

            block_iterator operator++(int) {
               block_iterator result(*this);
               ++(*this);
               return result;
            }

            block_iterator operator--(int) {
               block_iterator result(*this);
               --(*this);
               return result;
            }
         };

         using block_reverse_iterator = std::reverse_iterator<block_iterator>;

         // the four begin/end functions below can only be called in read-only mode
         block_iterator begin_block()const;
         block_iterator end_block()const;
         block_reverse_iterator rbegin_block()const { return std::make_reverse_iterator( begin_block() ); }
         block_reverse_iterator rend_blocks()const   { return std::make_reverse_iterator( end_block() ); }

         bool is_open()const;
         bool is_read_only()const;

         const boost::filesystem::path& get_path()const;

         /** @returns the block number of the last committed block in the log or 0 if no blocks are committed in the log */
         uint32_t last_committed_block_num()const;

         struct intrinsic_differences {
            uint32_t                        block_num = 0;
            transaction_id_type             trx_id;
            uint64_t                        global_sequence_num = 0;
            name                            receiver{};
            name                            first_receiver{};
            name                            action_name{};
            std::vector<intrinsic_record>   lhs_recorded_intrinsics;
            std::vector<intrinsic_record>   rhs_recorded_intrinsics;
         };

         /**
          * @pre requires both lhs and rhs to both have the same blocks, transactions, and actions (otherwise it throws)
          * @return empty optional if no difference; otherwise the intrinsic_differences for the first different action
          */
         static std::optional<intrinsic_differences>
         find_first_difference( intrinsic_debug_log& lhs, intrinsic_debug_log& rhs );

      private:
         std::unique_ptr<detail::intrinsic_debug_log_impl> my;
   };

} }

FC_REFLECT( eosio::chain::intrinsic_debug_log::intrinsic_record, (intrinsic_ordinal)(arguments_hash)(memory_hash) )
FC_REFLECT( eosio::chain::intrinsic_debug_log::action_data,
            (global_sequence_num)(receiver)(first_receiver)(action_name)(recorded_intrinsics)
)
FC_REFLECT( eosio::chain::intrinsic_debug_log::transaction_data, (trx_id)(actions) )
FC_REFLECT( eosio::chain::intrinsic_debug_log::block_data, (block_num)(transactions) )
FC_REFLECT( eosio::chain::intrinsic_debug_log::intrinsic_differences,
            (block_num)(trx_id)(global_sequence_num)(receiver)(first_receiver)(action_name)
            (lhs_recorded_intrinsics)(rhs_recorded_intrinsics)
)
