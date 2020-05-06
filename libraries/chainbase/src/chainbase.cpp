#include <chainbase/chainbase.hpp>
#include <boost/array.hpp>

#include <iostream>

#ifndef _WIN32
#include <sys/mman.h>
#endif

namespace chainbase {

   database::database(const bfs::path& dir, open_flags flags, uint64_t shared_file_size, bool allow_dirty,
                      pinnable_mapped_file::map_mode db_map_mode, std::vector<std::string> hugepage_paths ) :
      _db_file(dir, flags & database::read_write, shared_file_size, allow_dirty, db_map_mode, hugepage_paths),
      _read_only(flags == database::read_only)
   {
   }

   database::~database()
   {
      _index_list.clear();
      _index_map.clear();
   }

   void database::set_require_locking( bool enable_require_locking )
   {
#ifdef CHAINBASE_CHECK_LOCKING
      _enable_require_locking = enable_require_locking;
#endif
   }

#ifdef CHAINBASE_CHECK_LOCKING
   void database::require_lock_fail( const char* method, const char* lock_type, const char* tname )const
   {
      std::string err_msg = "database::" + std::string( method ) + " require_" + std::string( lock_type ) + "_lock() failed on type " + std::string( tname );
      std::cerr << err_msg << std::endl;
      BOOST_THROW_EXCEPTION( std::runtime_error( err_msg ) );
   }
#endif

   void database::undo()
   {
      for( auto& item : _index_list )
      {
         item->undo();
      }
   }

   void database::squash()
   {
      for( auto& item : _index_list )
      {
         item->squash();
      }
   }

   void database::commit( int64_t revision )
   {
      for( auto& item : _index_list )
      {
         item->commit( revision );
      }
   }

   void database::undo_all()
   {
      for( auto& item : _index_list )
      {
         item->undo_all();
      }
   }

   database::session database::start_undo_session( bool enabled )
   {
      if( enabled ) {
         vector< std::unique_ptr<abstract_session> > _sub_sessions;
         _sub_sessions.reserve( _index_list.size() );
         for( auto& item : _index_list ) {
            _sub_sessions.push_back( item->start_undo_session( enabled ) );
         }
         return session( std::move( _sub_sessions ) );
      } else {
         return session();
      }
   }

}  // namespace chainbase
