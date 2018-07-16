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


   struct by_block_id;
   struct by_block_num;
   struct by_lib_block_num;
   struct by_prev;
   typedef multi_index_container<
      block_state_ptr,
      indexed_by<
         hashed_unique< tag<by_block_id>, member<block_header_state, block_id_type, &block_header_state::id>, std::hash<block_id_type>>,
         ordered_non_unique< tag<by_prev>, const_mem_fun<block_header_state, const block_id_type&, &block_header_state::prev> >,
         ordered_non_unique< tag<by_block_num>,
            composite_key< block_state,
               member<block_header_state,uint32_t,&block_header_state::block_num>,
               member<block_state,bool,&block_state::in_current_chain>
            >,
            composite_key_compare< std::less<uint32_t>, std::greater<bool> >
         >,
         ordered_non_unique< tag<by_lib_block_num>,
            composite_key< block_header_state,
                member<block_header_state,uint32_t,&block_header_state::dpos_irreversible_blocknum>,
                member<block_header_state,uint32_t,&block_header_state::bft_irreversible_blocknum>,
                member<block_header_state,uint32_t,&block_header_state::block_num>
            >,
            composite_key_compare< std::greater<uint32_t>, std::greater<uint32_t>, std::greater<uint32_t> >
         >
      >
   > fork_multi_index_type;


   struct fork_database_impl {
      fork_multi_index_type index;
      block_state_ptr       head;
      fc::path              datadir;
   };


   fork_database::fork_database( const fc::path& data_dir ):my( new fork_database_impl() ) {
      my->datadir = data_dir;

      if (!fc::is_directory(my->datadir))
         fc::create_directories(my->datadir);

      auto fork_db_dat = my->datadir / config::forkdb_filename;
      if( fc::exists( fork_db_dat ) ) {
         string content;
         fc::read_file_contents( fork_db_dat, content );

         fc::datastream<const char*> ds( content.data(), content.size() );
         unsigned_int size; fc::raw::unpack( ds, size );
         for( uint32_t i = 0, n = size.value; i < n; ++i ) {
            block_state s;
            fc::raw::unpack( ds, s );
            set( std::make_shared<block_state>( move( s ) ) );
         }
         block_id_type head_id;
         fc::raw::unpack( ds, head_id );

         my->head = get_block( head_id );

         fc::remove( fork_db_dat );
      }
   }

   void fork_database::close() {
      if( my->index.size() == 0 ) return;

      auto fork_db_dat = my->datadir / config::forkdb_filename;
      std::ofstream out( fork_db_dat.generic_string().c_str(), std::ios::out | std::ios::binary | std::ofstream::trunc );
      uint32_t num_blocks_in_fork_db = my->index.size();
      fc::raw::pack( out, unsigned_int{num_blocks_in_fork_db} );
      for( const auto& s : my->index ) {
         fc::raw::pack( out, *s );
      }
      if( my->head )
         fc::raw::pack( out, my->head->id );
      else
         fc::raw::pack( out, block_id_type() );

      /// we don't normally indicate the head block as irreversible
      /// we cannot normally prune the lib if it is the head block because
      /// the next block needs to build off of the head block. We are exiting
      /// now so we can prune this block as irreversible before exiting.
      auto lib    = my->head->dpos_irreversible_blocknum;
      auto oldest = *my->index.get<by_block_num>().begin();
      if( oldest->block_num <= lib ) {
         prune( oldest );
      }

      my->index.clear();
   }

   fork_database::~fork_database() {
      close();
   }

   void fork_database::set( block_state_ptr s ) {
      auto result = my->index.insert( s );
      EOS_ASSERT( s->id == s->header.id(), fork_database_exception, 
                  "block state id (${id}) is different from block state header id (${hid})", ("id", string(s->id))("hid", string(s->header.id())) );

         //FC_ASSERT( s->block_num == s->header.block_num() );

      EOS_ASSERT( result.second, fork_database_exception, "unable to insert block state, duplicate state detected" );
      if( !my->head ) {
         my->head =  s;
      } else if( my->head->block_num < s->block_num ) {
         my->head =  s;
      }
   }

   block_state_ptr fork_database::add( block_state_ptr n ) {
      auto inserted = my->index.insert(n);
      EOS_ASSERT( inserted.second, fork_database_exception, "duplicate block added?" );

      my->head = *my->index.get<by_lib_block_num>().begin();

      auto lib    = my->head->dpos_irreversible_blocknum;
      auto oldest = *my->index.get<by_block_num>().begin();

      if( oldest->block_num < lib ) {
         prune( oldest );
      }

      return n;
   }

   block_state_ptr fork_database::add( signed_block_ptr b, bool trust ) {
      EOS_ASSERT( b, fork_database_exception, "attempt to add null block" );
      EOS_ASSERT( my->head, fork_db_block_not_found, "no head block set" );

      const auto& by_id_idx = my->index.get<by_block_id>();
      auto existing = by_id_idx.find( b->id() );
      EOS_ASSERT( existing == by_id_idx.end(), fork_database_exception, "we already know about this block" );

      auto prior = by_id_idx.find( b->previous );
      EOS_ASSERT( prior != by_id_idx.end(), unlinkable_block_exception, "unlinkable block", ("id", string(b->id()))("previous", string(b->previous)) );

      auto result = std::make_shared<block_state>( **prior, move(b), trust );
      EOS_ASSERT( result, fork_database_exception , "fail to add new block state" );
      return add(result);
   }

   const block_state_ptr& fork_database::head()const { return my->head; }

   /**
    *  Given two head blocks, return two branches of the fork graph that
    *  end with a common ancestor (same prior block)
    */
   pair< branch_type, branch_type >  fork_database::fetch_branch_from( const block_id_type& first,
                                                                       const block_id_type& second )const {
      pair<branch_type,branch_type> result;
      auto first_branch = get_block(first);
      auto second_branch = get_block(second);

      while( first_branch->block_num > second_branch->block_num )
      {
         result.first.push_back(first_branch);
         first_branch = get_block( first_branch->header.previous );
         EOS_ASSERT( first_branch, fork_db_block_not_found, "block ${id} does not exist", ("id", string(first_branch->header.previous)) );
      }

      while( second_branch->block_num > first_branch->block_num )
      {
         result.second.push_back( second_branch );
         second_branch = get_block( second_branch->header.previous );
         EOS_ASSERT( second_branch, fork_db_block_not_found, "block ${id} does not exist", ("id", string(second_branch->header.previous)) );
      }

      while( first_branch->header.previous != second_branch->header.previous )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
         first_branch = get_block( first_branch->header.previous );
         second_branch = get_block( second_branch->header.previous );
         EOS_ASSERT( first_branch && second_branch, fork_db_block_not_found, 
                     "either block ${fid} or ${sid} does not exist", 
                     ("fid", string(first_branch->header.previous))("sid", string(second_branch->header.previous)) );
      }

      if( first_branch && second_branch )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
      }
      return result;
   } /// fetch_branch_from

   /// remove all of the invalid forks built of this id including this id
   void fork_database::remove( const block_id_type& id ) {
      vector<block_id_type> remove_queue{id};

      for( uint32_t i = 0; i < remove_queue.size(); ++i ) {
         auto itr = my->index.find( remove_queue[i] );
         if( itr != my->index.end() )
            my->index.erase(itr);

         auto& previdx = my->index.get<by_prev>();
         auto  previtr = previdx.lower_bound(remove_queue[i]);
         while( previtr != previdx.end() && (*previtr)->header.previous == remove_queue[i] ) {
            remove_queue.push_back( (*previtr)->id );
            ++previtr;
         }
      }
      //wdump((my->index.size()));
      my->head = *my->index.get<by_lib_block_num>().begin();
   }

   void fork_database::set_validity( const block_state_ptr& h, bool valid ) {
      if( !valid ) {
         remove( h->id );
      } else {
         /// remove older than irreversible and mark block as valid
         h->validated = true;
      }
   }

   void fork_database::mark_in_current_chain( const block_state_ptr& h, bool in_current_chain ) {
      if( h->in_current_chain == in_current_chain )
         return;

      auto& by_id_idx = my->index.get<by_block_id>();
      auto itr = by_id_idx.find( h->id );
      EOS_ASSERT( itr != by_id_idx.end(), fork_db_block_not_found, "could not find block in fork database" );

      by_id_idx.modify( itr, [&]( auto& bsp ) { // Need to modify this way rather than directly so that Boost MultiIndex can re-sort
         bsp->in_current_chain = in_current_chain;
      });
   }

   void fork_database::prune( const block_state_ptr& h ) {
      auto num = h->block_num;

      auto& by_bn = my->index.get<by_block_num>();
      auto bni = by_bn.begin();
      while( bni != by_bn.end() && (*bni)->block_num < num ) {
         prune( *bni );
         bni = by_bn.begin();
      }

      auto itr = my->index.find( h->id );
      if( itr != my->index.end() ) {
         irreversible(*itr);
         my->index.erase(itr);
      }

      auto& numidx = my->index.get<by_block_num>();
      auto nitr = numidx.lower_bound( num );
      while( nitr != numidx.end() && (*nitr)->block_num == num ) {
         auto itr_to_remove = nitr;
         ++nitr;
         auto id = (*itr_to_remove)->id;
         remove( id );
      }
   }

   block_state_ptr   fork_database::get_block(const block_id_type& id)const {
      auto itr = my->index.find( id );
      if( itr != my->index.end() )
         return *itr;
      return block_state_ptr();
   }

   block_state_ptr   fork_database::get_block_in_current_chain_by_num( uint32_t n )const {
      const auto& numidx = my->index.get<by_block_num>();
      auto nitr = numidx.lower_bound( n );
      // following asserts removed so null can be returned
      //FC_ASSERT( nitr != numidx.end() && (*nitr)->block_num == n,
      //           "could not find block in fork database with block number ${block_num}", ("block_num", n) );
      //FC_ASSERT( (*nitr)->in_current_chain == true,
      //           "block (with block number ${block_num}) found in fork database is not in the current chain", ("block_num", n) );
      if( nitr == numidx.end() || (*nitr)->block_num != n || (*nitr)->in_current_chain != true )
         return block_state_ptr();
      return *nitr;
   }

   void fork_database::add( const header_confirmation& c ) {
      auto b = get_block( c.block_id );
      EOS_ASSERT( b, fork_db_block_not_found, "unable to find block id ${id}", ("id",c.block_id));
      b->add_confirmation( c );

      if( b->bft_irreversible_blocknum < b->block_num &&
         b->confirmations.size() > ((b->active_schedule.producers.size() * 2) / 3) ) {
         set_bft_irreversible( c.block_id );
      }
   }

   /**
    *  This method will set this block as being BFT irreversible and will update
    *  all blocks which build off of it to have the same bft_irb if their existing
    *  bft irb is less than this block num.
    *
    *  This will require a search over all forks
    */
   void fork_database::set_bft_irreversible( block_id_type id ) {
      auto& idx = my->index.get<by_block_id>();
      auto itr = idx.find(id);
      uint32_t block_num = (*itr)->block_num;
      idx.modify( itr, [&]( auto& bsp ) {
           bsp->bft_irreversible_blocknum = bsp->block_num;
      });

      /** to prevent stack-overflow, we perform a bredth-first traversal of the
       * fork database. At each stage we iterate over the leafs from the prior stage
       * and find all nodes that link their previous. If we update the bft lib then we
       * add it to a queue for the next layer.  This lambda takes one layer and returns
       * all block ids that need to be iterated over for next layer.
       */
      auto update = [&]( const vector<block_id_type>& in ) {
         vector<block_id_type> updated;

         for( const auto& i : in ) {
            auto& pidx = my->index.get<by_prev>();
            auto pitr  = pidx.lower_bound( i );
            auto epitr = pidx.upper_bound( i );
            while( pitr != epitr ) {
               pidx.modify( pitr, [&]( auto& bsp ) {
                 if( bsp->bft_irreversible_blocknum < block_num ) {
                    bsp->bft_irreversible_blocknum = block_num;
                    updated.push_back( bsp->id );
                 }
               });
               ++pitr;
            }
         }
         return updated;
      };

      vector<block_id_type> queue{id};
      while( queue.size() ) {
         queue = update( queue );
      }
   }

} } /// eosio::chain
