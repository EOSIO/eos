#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/io/fstream.hpp>
#include <fstream>

namespace eosio { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;

   const uint32_t fork_database::magic_number = 0x30510FDB;

   const uint32_t fork_database::min_supported_version = 1;
   const uint32_t fork_database::max_supported_version = 1;

   /**
    * History:
    * Version 1: initial version of the new refactored fork database portable format
    */

   struct by_block_id;
   struct by_block_num;
   struct by_lib_block_num;
   struct by_prev;
   typedef multi_index_container<
      block_state_ptr,
      indexed_by<
         hashed_unique< tag<by_block_id>, member<block_header_state, block_id_type, &block_header_state::id>, std::hash<block_id_type>>,
         ordered_non_unique< tag<by_prev>, const_mem_fun<block_header_state, const block_id_type&, &block_header_state::prev> >,
         ordered_unique< tag<by_lib_block_num>,
            composite_key< block_state,
               member<block_state,                        bool,          &block_state::validated>,
               member<detail::block_header_state_common, uint32_t,      &detail::block_header_state_common::dpos_irreversible_blocknum>,
               member<detail::block_header_state_common, uint32_t,      &detail::block_header_state_common::block_num>,
               member<block_header_state,                block_id_type, &block_header_state::id>
            >,
            composite_key_compare<
               std::greater<bool>,
               std::greater<uint32_t>,
               std::greater<uint32_t>,
               sha256_less
            >
         >
      >
   > fork_multi_index_type;

   bool first_preferred( const block_header_state& lhs, const block_header_state& rhs ) {
      return std::tie( lhs.dpos_irreversible_blocknum, lhs.block_num )
               > std::tie( rhs.dpos_irreversible_blocknum, rhs.block_num );
   }

   struct fork_database_impl {
      fork_database_impl( fork_database& self, const fc::path& data_dir )
      :self(self)
      ,datadir(data_dir)
      {}

      fork_database&        self;
      fork_multi_index_type index;
      block_state_ptr       root; // Only uses the block_header_state portion
      block_state_ptr       head;
      fc::path              datadir;

      void add( const block_state_ptr& n,
                bool ignore_duplicate, bool validate,
                const std::function<void( block_timestamp_type,
                                          const flat_set<digest_type>&,
                                          const vector<digest_type>& )>& validator );
   };


   fork_database::fork_database( const fc::path& data_dir )
   :my( new fork_database_impl( *this, data_dir ) )
   {}


   void fork_database::open( const std::function<void( block_timestamp_type,
                                                       const flat_set<digest_type>&,
                                                       const vector<digest_type>& )>& validator )
   {
      if (!fc::is_directory(my->datadir))
         fc::create_directories(my->datadir);

      auto fork_db_dat = my->datadir / config::forkdb_filename;
      if( fc::exists( fork_db_dat ) ) {
         try {
            string content;
            fc::read_file_contents( fork_db_dat, content );

            fc::datastream<const char*> ds( content.data(), content.size() );

            // validate totem
            uint32_t totem = 0;
            fc::raw::unpack( ds, totem );
            EOS_ASSERT( totem == magic_number, fork_database_exception,
                        "Fork database file '${filename}' has unexpected magic number: ${actual_totem}. Expected ${expected_totem}",
                        ("filename", fork_db_dat.generic_string())
                        ("actual_totem", totem)
                        ("expected_totem", magic_number)
            );

            // validate version
            uint32_t version = 0;
            fc::raw::unpack( ds, version );
            EOS_ASSERT( version >= min_supported_version && version <= max_supported_version,
                        fork_database_exception,
                       "Unsupported version of fork database file '${filename}'. "
                       "Fork database version is ${version} while code supports version(s) [${min},${max}]",
                       ("filename", fork_db_dat.generic_string())
                       ("version", version)
                       ("min", min_supported_version)
                       ("max", max_supported_version)
            );

            block_header_state bhs;
            fc::raw::unpack( ds, bhs );
            reset( bhs );

            unsigned_int size; fc::raw::unpack( ds, size );
            for( uint32_t i = 0, n = size.value; i < n; ++i ) {
               block_state s;
               fc::raw::unpack( ds, s );
               // do not populate transaction_metadatas, they will be created as needed in apply_block with appropriate key recovery
               s.trxs.clear(); // not part of fc_reflect, so should be empty here
               s.header_exts = s.block->validate_and_extract_header_extensions();
               my->add( std::make_shared<block_state>( move( s ) ), false, true, validator );
            }
            block_id_type head_id;
            fc::raw::unpack( ds, head_id );

            if( my->root->id == head_id ) {
               my->head = my->root;
            } else {
               my->head = get_block( head_id );
               EOS_ASSERT( my->head, fork_database_exception,
                           "could not find head while reconstructing fork database from file; '${filename}' is likely corrupted",
                           ("filename", fork_db_dat.generic_string()) );
            }

            auto candidate = my->index.get<by_lib_block_num>().begin();
            if( candidate == my->index.get<by_lib_block_num>().end() || !(*candidate)->is_valid() ) {
               EOS_ASSERT( my->head->id == my->root->id, fork_database_exception,
                           "head not set to root despite no better option available; '${filename}' is likely corrupted",
                           ("filename", fork_db_dat.generic_string()) );
            } else {
               EOS_ASSERT( !first_preferred( **candidate, *my->head ), fork_database_exception,
                           "head not set to best available option available; '${filename}' is likely corrupted",
                           ("filename", fork_db_dat.generic_string()) );
            }
         } FC_CAPTURE_AND_RETHROW( (fork_db_dat) )

         fc::remove( fork_db_dat );
      }
   }

   void fork_database::close() {
      auto fork_db_dat = my->datadir / config::forkdb_filename;

      if( !my->root ) {
         if( my->index.size() > 0 ) {
            elog( "fork_database is in a bad state when closing; not writing out '${filename}'",
                  ("filename", fork_db_dat.generic_string()) );
         }
         return;
      }

      std::ofstream out( fork_db_dat.generic_string().c_str(), std::ios::out | std::ios::binary | std::ofstream::trunc );
      fc::raw::pack( out, magic_number );
      fc::raw::pack( out, max_supported_version ); // write out current version which is always max_supported_version
      fc::raw::pack( out, *static_cast<block_header_state*>(&*my->root) );
      uint32_t num_blocks_in_fork_db = my->index.size();
      fc::raw::pack( out, unsigned_int{num_blocks_in_fork_db} );

      const auto& indx = my->index.get<by_lib_block_num>();

      auto unvalidated_itr = indx.rbegin();
      auto unvalidated_end = boost::make_reverse_iterator( indx.lower_bound( false ) );

      auto validated_itr = unvalidated_end;
      auto validated_end = indx.rend();

      for(  bool unvalidated_remaining = (unvalidated_itr != unvalidated_end),
                 validated_remaining   = (validated_itr != validated_end);

            unvalidated_remaining || validated_remaining;

            unvalidated_remaining = (unvalidated_itr != unvalidated_end),
            validated_remaining   = (validated_itr != validated_end)
         )
      {
         auto itr = (validated_remaining ? validated_itr : unvalidated_itr);

         if( unvalidated_remaining && validated_remaining ) {
            if( first_preferred( **validated_itr, **unvalidated_itr ) ) {
               itr = unvalidated_itr;
               ++unvalidated_itr;
            } else {
               ++validated_itr;
            }
         } else if( unvalidated_remaining ) {
            ++unvalidated_itr;
         } else {
            ++validated_itr;
         }

         fc::raw::pack( out, *(*itr) );
      }

      if( my->head ) {
         fc::raw::pack( out, my->head->id );
      } else {
         elog( "head not set in fork database; '${filename}' will be corrupted",
               ("filename", fork_db_dat.generic_string()) );
      }

      my->index.clear();
   }

   fork_database::~fork_database() {
      close();
   }

   void fork_database::reset( const block_header_state& root_bhs ) {
      my->index.clear();
      my->root = std::make_shared<block_state>();
      static_cast<block_header_state&>(*my->root) = root_bhs;
      my->root->validated = true;
      my->head = my->root;
   }

   void fork_database::rollback_head_to_root() {
      auto& by_id_idx = my->index.get<by_block_id>();
      auto itr = by_id_idx.begin();
      while (itr != by_id_idx.end()) {
         by_id_idx.modify( itr, [&]( block_state_ptr& bsp ) {
            bsp->validated = false;
         } );
         ++itr;
      }
      my->head = my->root;
   }

   void fork_database::advance_root( const block_id_type& id ) {
      EOS_ASSERT( my->root, fork_database_exception, "root not yet set" );

      auto new_root = get_block( id );
      EOS_ASSERT( new_root, fork_database_exception,
                  "cannot advance root to a block that does not exist in the fork database" );
      EOS_ASSERT( new_root->is_valid(), fork_database_exception,
                  "cannot advance root to a block that has not yet been validated" );


      vector<block_id_type> blocks_to_remove;
      for( auto b = new_root; b; ) {
         blocks_to_remove.push_back( b->header.previous );
         b = get_block( blocks_to_remove.back() );
         EOS_ASSERT( b || blocks_to_remove.back() == my->root->id, fork_database_exception, "invariant violation: orphaned branch was present in forked database" );
      }

      // The new root block should be erased from the fork database index individually rather than with the remove method,
      // because we do not want the blocks branching off of it to be removed from the fork database.
      my->index.erase( my->index.find( id ) );

      // The other blocks to be removed are removed using the remove method so that orphaned branches do not remain in the fork database.
      for( const auto& block_id : blocks_to_remove ) {
         remove( block_id );
      }

      // Even though fork database no longer needs block or trxs when a block state becomes a root of the tree,
      // avoid mutating the block state at all, for example clearing the block shared pointer, because other
      // parts of the code which run asynchronously (e.g. mongo_db_plugin) may later expect it remain unmodified.

      my->root = new_root;
   }

   block_header_state_ptr fork_database::get_block_header( const block_id_type& id )const {
      const auto& by_id_idx = my->index.get<by_block_id>();

      if( my->root->id == id ) {
         return my->root;
      }

      auto itr = my->index.find( id );
      if( itr != my->index.end() )
         return *itr;

      return block_header_state_ptr();
   }

   void fork_database_impl::add( const block_state_ptr& n,
                                 bool ignore_duplicate, bool validate,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator )
   {
      EOS_ASSERT( root, fork_database_exception, "root not yet set" );
      EOS_ASSERT( n, fork_database_exception, "attempt to add null block state" );

      auto prev_bh = self.get_block_header( n->header.previous );

      EOS_ASSERT( prev_bh, unlinkable_block_exception,
                  "unlinkable block", ("id", n->id)("previous", n->header.previous) );

      if( validate ) {
         try {
            const auto& exts = n->header_exts;

            if( exts.count(protocol_feature_activation::extension_id()) > 0 ) {
               const auto& new_protocol_features = exts.lower_bound(protocol_feature_activation::extension_id())->second.get<protocol_feature_activation>().protocol_features;
               validator( n->header.timestamp, prev_bh->activated_protocol_features->protocol_features, new_protocol_features );
            }
         } EOS_RETHROW_EXCEPTIONS( fork_database_exception, "serialized fork database is incompatible with configured protocol features"  )
      }

      auto inserted = index.insert(n);
      if( !inserted.second ) {
         if( ignore_duplicate ) return;
         EOS_THROW( fork_database_exception, "duplicate block added", ("id", n->id) );
      }

      auto candidate = index.get<by_lib_block_num>().begin();
      if( (*candidate)->is_valid() ) {
         head = *candidate;
      }
   }

   void fork_database::add( const block_state_ptr& n, bool ignore_duplicate ) {
      my->add( n, ignore_duplicate, false,
               []( block_timestamp_type timestamp,
                   const flat_set<digest_type>& cur_features,
                   const vector<digest_type>& new_features )
               {}
      );
   }

   const block_state_ptr& fork_database::root()const { return my->root; }

   const block_state_ptr& fork_database::head()const { return my->head; }

   block_state_ptr fork_database::pending_head()const {
      const auto& indx = my->index.get<by_lib_block_num>();

      auto itr = indx.lower_bound( false );
      if( itr != indx.end() && !(*itr)->is_valid() ) {
         if( first_preferred( **itr, *my->head ) )
            return *itr;
      }

      return my->head;
   }

   branch_type fork_database::fetch_branch( const block_id_type& h, uint32_t trim_after_block_num )const {
      branch_type result;
      for( auto s = get_block(h); s; s = get_block( s->header.previous ) ) {
         if( s->block_num <= trim_after_block_num )
             result.push_back( s );
      }

      return result;
   }

   block_state_ptr fork_database::search_on_branch( const block_id_type& h, uint32_t block_num )const {
      for( auto s = get_block(h); s; s = get_block( s->header.previous ) ) {
         if( s->block_num == block_num )
             return s;
      }

      return {};
   }

   /**
    *  Given two head blocks, return two branches of the fork graph that
    *  end with a common ancestor (same prior block)
    */
   pair< branch_type, branch_type >  fork_database::fetch_branch_from( const block_id_type& first,
                                                                       const block_id_type& second )const {
      pair<branch_type,branch_type> result;
      auto first_branch = (first == my->root->id) ? my->root : get_block(first);
      auto second_branch = (second == my->root->id) ? my->root : get_block(second);

      EOS_ASSERT(first_branch, fork_db_block_not_found, "block ${id} does not exist", ("id", first));
      EOS_ASSERT(second_branch, fork_db_block_not_found, "block ${id} does not exist", ("id", second));

      while( first_branch->block_num > second_branch->block_num )
      {
         result.first.push_back(first_branch);
         const auto& prev = first_branch->header.previous;
         first_branch = (prev == my->root->id) ? my->root : get_block( prev );
         EOS_ASSERT( first_branch, fork_db_block_not_found,
                     "block ${id} does not exist",
                     ("id", prev)
         );
      }

      while( second_branch->block_num > first_branch->block_num )
      {
         result.second.push_back( second_branch );
         const auto& prev = second_branch->header.previous;
         second_branch = (prev == my->root->id) ? my->root : get_block( prev );
         EOS_ASSERT( second_branch, fork_db_block_not_found,
                     "block ${id} does not exist",
                     ("id", prev)
         );
      }

      if (first_branch->id == second_branch->id) return result;

      while( first_branch->header.previous != second_branch->header.previous )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
         const auto &first_prev = first_branch->header.previous;
         first_branch = get_block( first_prev );
         const auto &second_prev = second_branch->header.previous;
         second_branch = get_block( second_prev );
         EOS_ASSERT( first_branch, fork_db_block_not_found,
                     "block ${id} does not exist",
                     ("id", first_prev)
         );
         EOS_ASSERT( second_branch, fork_db_block_not_found,
                     "block ${id} does not exist",
                     ("id", second_prev)
         );
      }

      if( first_branch && second_branch )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
      }
      return result;
   } /// fetch_branch_from

   /// remove all of the invalid forks built off of this id including this id
   void fork_database::remove( const block_id_type& id ) {
      vector<block_id_type> remove_queue{id};
      const auto& previdx = my->index.get<by_prev>();
      const auto head_id = my->head->id;

      for( uint32_t i = 0; i < remove_queue.size(); ++i ) {
         EOS_ASSERT( remove_queue[i] != head_id, fork_database_exception,
                     "removing the block and its descendants would remove the current head block" );

         auto previtr = previdx.lower_bound( remove_queue[i] );
         while( previtr != previdx.end() && (*previtr)->header.previous == remove_queue[i] ) {
            remove_queue.push_back( (*previtr)->id );
            ++previtr;
         }
      }

      for( const auto& block_id : remove_queue ) {
         auto itr = my->index.find( block_id );
         if( itr != my->index.end() )
            my->index.erase(itr);
      }
   }

   void fork_database::mark_valid( const block_state_ptr& h ) {
      if( h->validated ) return;

      auto& by_id_idx = my->index.get<by_block_id>();

      auto itr = by_id_idx.find( h->id );
      EOS_ASSERT( itr != by_id_idx.end(), fork_database_exception,
                  "block state not in fork database; cannot mark as valid",
                  ("id", h->id) );

      by_id_idx.modify( itr, []( block_state_ptr& bsp ) {
         bsp->validated = true;
      } );

      auto candidate = my->index.get<by_lib_block_num>().begin();
      if( first_preferred( **candidate, *my->head ) ) {
         my->head = *candidate;
      }
   }

   block_state_ptr   fork_database::get_block(const block_id_type& id)const {
      auto itr = my->index.find( id );
      if( itr != my->index.end() )
         return *itr;
      return block_state_ptr();
   }

} } /// eosio::chain
