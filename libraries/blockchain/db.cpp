#include <eosio/blockchain/db.hpp>

namespace eosio { namespace blockchain {

struct environment_check {
   environment_check() {
      memset( &compiler_version, 0, sizeof( compiler_version ) );
      memcpy( &compiler_version, __VERSION__, std::min<size_t>( strlen(__VERSION__), 256 ) );
#ifndef NDEBUG
      debug = true;
#endif
#ifdef __APPLE__
      apple = true;
#endif
#ifdef WIN32
      windows = true;
#endif
   }
   friend bool operator == ( const environment_check& a, const environment_check& b ) {
      return std::make_tuple( a.compiler_version, a.debug, a.apple, a.windows )
         ==  std::make_tuple( b.compiler_version, b.debug, b.apple, b.windows );
   }

   boost::array<char,256>  compiler_version;
   bool                    debug = false;
   bool                    apple = false;
   bool                    windows = false;
};

database::database( const path& dir, uint64_t shared_file_size ) {
   _data_dir = dir;
   bfs::create_directories(dir);
   auto abs_path = bfs::absolute( dir / "shared_memory.bin" );

   if( bfs::exists( abs_path ) )
   {
      if( true /*write*/ )
      {
         auto existing_file_size = bfs::file_size( abs_path );
         if( shared_file_size > existing_file_size )
         {
            if( !bip::managed_mapped_file::grow( abs_path.generic_string().c_str(), shared_file_size - existing_file_size ) )
               BOOST_THROW_EXCEPTION( std::runtime_error( "could not grow database file to requested size." ) );
         }

         _segment.reset( new bip::managed_mapped_file( bip::open_only,
                                                       abs_path.generic_string().c_str()
                                                       ) );

         auto env = _segment->find< environment_check >( "environment" );
         if( !env.first || !( *env.first == environment_check()) ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "database created by a different compiler, build, or operating system" ) );
         }

         _scopes = _segment->find< scopes_type  >( "scopes" ).first;
         if( !_scopes ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find 'scopes' in shared memory file" ) );
         }
         _block_undo_history = _segment->find< shared_deque<undo_block>  >( "block_undo_stack" ).first;
         if( !_block_undo_history ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find 'block_undo_stack' in shared memory file" ) );
         }

      } 
   } else { // no shared memory file exists
      FC_ASSERT( shared_file_size > 0, "attempting to create a database with size 0" );
      ilog( "creating new database file: ${db}", ("db", abs_path.generic_string()) );
      _segment.reset( new bip::managed_mapped_file( bip::create_only,
                                                    abs_path.generic_string().c_str(), shared_file_size
                                                    ) );
      _segment->find_or_construct< environment_check >( "environment" )();

      _scopes = _segment->find_or_construct< scopes_type  >( "scopes" )( _segment->get_segment_manager() ); 
      _block_undo_history = _segment->find_or_construct< shared_deque<undo_block>  >( "block_undo_stack" )( _segment->get_segment_manager() ); 
   }
}

database::~database() {
} // ~database

/**
 * @return -1 if there is no undo history, otherwise returns the first block for which there exists an undo state
 */
block_num_type database::last_reversible_block() const {
   if( _block_undo_history->size() == 0 ) return block_num_type(-1);
   return _block_undo_history->front()._block_num;
}

/**
 * Starts a new block, if a current undo history exists it must be a sequential increment on the current block number,
 * otherwise, if the last_reversible_block == max_block_num then it can be any block number.
 */
block_handle database::start_block( block_num_type num ) {
   if( last_reversible_block() != max_block_num ) {
      auto& head_udb = _block_undo_history->back();
      FC_ASSERT( head_udb._block_num + 1 == num, "new blocks must have sequential block numbers" );
   }
   _block_undo_history->emplace_back( _segment->get_segment_manager() );
   auto& udb = _block_undo_history->back();
   udb._block_num = num;

   return block_handle( *this, udb );
} /// start_block

cycle_handle block_handle::start_cycle() {
  _undo_block._cycle_undo_history.emplace_back( _db.get_allocator() );
  return cycle_handle( *this, _undo_block._cycle_undo_history.back() );
}


cycle_handle::cycle_handle( const block_handle& blk, undo_cycle& cyc )
:_db(blk._db),_undo_block(blk._undo_block),_undo_cycle(cyc){}

shard_handle cycle_handle::start_shard( const set<scope_name>& write, const set<scope_name>& read ) {
  /// TODO: verify there are no conflicts in scopes, then create new shard
  auto itr = _undo_cycle._shards.emplace( _undo_cycle._shards.begin(), _db.get_allocator(), write, read );
  return shard_handle( *this, *itr );
}

shard_handle::shard_handle( cycle_handle& c, shard& s )
:_db(c._db),_shard(s) {
}

transaction_handle shard_handle::start_transaction() {
   auto& hist = _shard._transaction_history;
   auto itr = hist.emplace( hist.end(), _db.get_allocator(), std::ref(_shard)  );

   return transaction_handle( *this, *itr );
}

shard::shard( allocator<char> a, const set<scope_name>& w, const set<scope_name>& r )
:_write_scope(w.begin(),w.end(),a),_read_scope(r.begin(),r.end(),a),_transaction_history(a){}


transaction_handle::transaction_handle( shard_handle& sh, undo_transaction& udt )
:_db(sh._db),_undo_trx(udt) {
}

scope_handle transaction_handle::create_scope( scope_name s ) {
   /// TODO: this method is only valid if we have scope-scope in the shard
   auto existing_scope = _db.find_scope( s ); 
   FC_ASSERT( !existing_scope, "scope ${s} already exists", ("s",s) );

   auto new_scope = _db.create_scope(s);
   _undo_trx.new_scopes.push_back(s);

   return scope_handle( *this, *new_scope );
}


scope_handle::scope_handle( transaction_handle& trx, scope& s )
:_scope(s),_trx(trx){}



scope* database::create_scope( scope_name n ) {
   auto result = _scopes->insert( pair<scope_name,scope>( n, get_allocator() ) );
   FC_ASSERT( result.second, "unable to insert new scope ${n}", ("n",n) );
   wlog( "created scope ${s}", ("s",name(n)));
   return &result.first->second;
}
void database::delete_scope( scope_name n ) {
  // auto itr = _scopes->find(n);
 //  FC_ASSERT( itr != _scopes->end(), "unable to find scope to delete it" );
    _scopes->erase(n);
   idump((name(n)));
}

const scope* database::find_scope( scope_name n )const {
   auto itr = _scopes->find( n );
   if( itr == _scopes->end() ) return nullptr;
   return &itr->second;
} // find_scope

const scope& database::get_scope( scope_name n )const {
   auto ptr = find_scope( n );
   FC_ASSERT( ptr != nullptr, "unable to find expected scope ${n}", ("n",name(n)) );
   return *ptr;
}


const table* scope::find_table( table_name t )const {
   auto itr = _tables.find(t);
   if( itr == _tables.end() ) return nullptr;
   return itr->second.get();
}

void transaction_handle::on_create_table( table& t ) {
   FC_ASSERT( _undo_trx.new_tables.insert(&t).second, "unable to insert table" );
}

void transaction_handle::undo() {
   _applied = true; 
   _db.undo( _undo_trx );
}

void block_handle::undo() {
   _db.undo( _undo_block ); 
}

/**
 *  After calling this @udt will be invalid
 */
void database::undo( undo_transaction& udt ) {
   /// this transaction should be part of a shard, undoing it should remove the udt from _transaction_history
   FC_ASSERT( &udt._shard->_transaction_history.back() == &udt, "must undo last transaction in shard first" );

   for( const auto& item : udt.new_tables ) {
      auto& t = *item;
      auto& s = *t._scope;
      s._tables.erase( t._name );
      get_abstract_table( t ).destruct( t, get_allocator() );
   }
   udt.new_tables.clear();

   for( const auto& s: udt.new_scopes ) {
      FC_ASSERT( get_scope(s)._tables.size() == 0, "all tables should be deleted before deleting scope" );
      delete_scope( s );
   }

   /// frees the memory referenced by udt
   udt._shard->_transaction_history.pop_back();
}

void database::undo( undo_block& udb ) {
   auto& last = _block_undo_history->back();
   FC_ASSERT( &last == &udb, "attempt to undo block other than head" );
   for( auto& c : udb._cycle_undo_history ) {
      for( auto& s : c._shards ) {
         while( s._transaction_history.size() ) {
            undo( s._transaction_history.back() );
         }
      }
   }
   wlog( "poping block ${b}", ("b", udb._block_num) );
   _block_undo_history->pop_back();
}

} } // eosio::blockchain
