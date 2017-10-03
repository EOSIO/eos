#include <eosio/blockchain/database.hpp>
#include <fc/log/logger.hpp>

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
         _undo_stack = _segment->find< shared_deque<undo_state>  >( "undo_stack" ).first;
         if( !_undo_stack ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find 'undo_stack' in shared memory file" ) );
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
      _undo_stack = _segment->find_or_construct< shared_deque<undo_state>  >( "undo_stack" )( _segment->get_segment_manager() ); 
   }
}

database::~database() {
} // ~database


void database::flush() {
   if( _segment )
      _segment->flush();
   if( _meta )
      _meta->flush();
} // flush

const scope_state* database::find_scope( scope_name scope )const {
   auto scope_itr = _scopes->find( scope );
   return scope_itr == _scopes->end() ? nullptr : scope_itr->second.get();
}

scope_state* database::find_scope( scope_name scope ) {
   auto scope_itr = _scopes->find( scope );
   return scope_itr == _scopes->end() ? nullptr : scope_itr->second.get();
}

const scope_state& database::get_scope( scope_name scope )const {
   auto existing_scope = find_scope( scope );
   FC_ASSERT( existing_scope, "unknown scope ${s}", ("s",scope) );
   return *existing_scope;
}

scope_state& database::get_scope( scope_name scope ) {
   auto existing_scope = find_scope( scope );
   FC_ASSERT( existing_scope, "unknown scope ${s}", ("s",scope) );
   return *existing_scope;
}


const scope_state& database::create_scope( scope_name scope ) {
   auto memory = _segment->get_segment_manager();

   auto scope_itr = _scopes->find( scope );
   FC_ASSERT( scope_itr == _scopes->end(), "scope with name ${n} already exists", ("n",scope));
   if( _scopes->end() == scope_itr ) {
      ilog( "creating scope state ${n}", ("n",scope) );
      auto state = _segment->construct<scope_state>( bip::anonymous_instance )( memory );
      scope_itr = _scopes->emplace( scope, state ).first;
      scope_itr->second->_name = scope;

      if( _undo_stack->size() ) {
         _undo_stack->back().new_scopes.emplace_back( state );
      }
   }
   return *scope_itr->second;
}
void database::delete_scope( scope_state& scope ) { 
   auto scope_name = scope._name;
   idump((scope_name));
   try {
      /** this would normally be in the scope_state destructor, except that
       * scope_state does not have access to database to get the virtual interfaces
       * for the different tables.
       */
      auto itr = scope._tables.begin();
      while( itr != scope._tables.end() ) {
         delete_table( *itr->second );
         itr = scope._tables.begin();
      }
      _segment->destroy_ptr( &scope );
      _scopes->erase( _scopes->find(scope_name) );
   } FC_CAPTURE_AND_RETHROW( (scope_name) ) 
}

void database::delete_table( const table& tbl ) {
   auto& tables = const_cast<scope_state&>(*tbl._scope)._tables;

   auto table_itr = tables.find( tbl._name );
   if( table_itr != tables.end() ) {
      table* tab = table_itr->second.get();
      auto& tab_interface = get_interface(tab->_type);
      tab_interface.destruct( tab, _segment->get_segment_manager() );
      tables.erase( table_itr );
   }
}

void database::undo_all() {
   auto& stack = *_undo_stack;
   while( stack.size() ) {
      undo();
   }
}

void database::undo() 
{ try{

   if( _undo_stack->size() == 0 ) 
      return;

   auto& changes = _undo_stack->back();
   auto prior_revision = changes.revision - 1;

   for( const auto& thread : changes.threads ) {
      for( const auto& mod_table : thread.modified_tables ) {
         get_interface(*mod_table.second).undo_until( *mod_table.second, prior_revision );
      }
      for( auto new_table : thread.new_tables )
         delete_table( get_scope( new_table.first ).get_table( new_table.second ) );
   }

   for( auto& new_scope : changes.new_scopes ) {
      delete_scope( *new_scope );
   }

   _undo_stack->pop_back();
   ilog( "now on revision ${r} plus changes", ("r",_undo_stack->size() ? _undo_stack->back().revision : 0) );
} FC_CAPTURE_AND_RETHROW() } // database::undo()


/**
 * Return the revision that we were on at the start of the current undo session
 */
uint64_t database::revision()const {
   return _undo_stack->size() == 0 ? 0 : _undo_stack->back().revision;
}

void database::set_revision( uint64_t revision ) {
   FC_ASSERT( _undo_stack->size() == 0, "revision has already been set" );
   _undo_stack->emplace_back( _segment->get_segment_manager() );
   _undo_stack->back().revision = revision;
}

database::thread_undo_state* database::find_thread_undo_state_for_scope( scope_name scope )const {
   if( !_undo_stack->size() ) return nullptr;
   auto& head = _undo_stack->back();
   auto itr = head.thread_undo_states.find( scope );
   if( itr == head.thread_undo_states.end() )
      return nullptr;
   return itr->second.get();
}

database::thread database::start_thread( const flat_set<scope_name>& scopes, const flat_set<scope_name>& read_scopes ) {
   FC_ASSERT( _undo_stack->size() );
   auto& head = _undo_stack->back();

   for( const auto& scope : scopes ) {
      auto tuds = find_thread_undo_state_for_scope( scope );
      FC_ASSERT( tuds == nullptr, "scope already in use by another thread" );
      FC_ASSERT( head.read_locks.find( scope ) == head.read_locks.end(), "scope ${n} is already locked for reading", ("n",scope) );
   }

   for( auto read_scope : read_scopes ) {
      FC_ASSERT( scopes.find(read_scope) == scopes.end(), "cannot ask for RO & RW for same scope: ${n}", ("n",read_scope) );
      auto tuds = find_thread_undo_state_for_scope( read_scope );
      FC_ASSERT( !tuds, "attempt to grab read scope for scope '${n}' that is already open with write lock", ("n",read_scope) );
      head.read_locks.insert(read_scope);
   }

   head.threads.emplace_back( _segment->get_segment_manager() );
   auto& tuds = head.threads.back();
   tuds.db_undo_state = &head;
   tuds.write_scopes.reserve(scopes.size());
   for( auto scope : scopes ) {
      head.thread_undo_states[scope] = &tuds;
      tuds.write_scopes.push_back(scope);
   }

   return thread( *this, tuds );
} /// start_thread

void database::push_revision(uint64_t revision) {
   if( _undo_stack->size() ) {
      FC_ASSERT( _undo_stack->back().revision < revision, "new revision must be higher than current revision" );
   }
   _undo_stack->emplace_back( _segment->get_segment_manager() );
   _undo_stack->back().revision = revision;
}

void database::pop_revision( uint64_t revision ) {
   while( _undo_stack->size() && _undo_stack->back().revision >= revision )
      undo();
}


} } /// eosio::blockchain
