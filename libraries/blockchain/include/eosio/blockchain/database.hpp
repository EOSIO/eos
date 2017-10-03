#pragma once
#include <eosio/blockchain/types.hpp>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

#include <boost/multi_index_container.hpp>

#include <boost/chrono.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>


namespace eosio { namespace blockchain {

   namespace bip = boost::interprocess;
   namespace bfs = boost::filesystem;
   using bip::offset_ptr;

   template<typename T>
   using allocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;

   template<typename T>
   using shared_deque = bip::deque<T, allocator<T> >;

   template<typename T>
   using shared_vector = std::vector<T, allocator<T> >;

   template<typename T>
   using shared_set = bip::set< T, std::less<T>, allocator<T> >;

   template<typename T>
   using shared_list = bip::list< T, allocator<T> >;

   template<typename Key, typename Value>
   using shared_map = bip::map< Key, Value, std::less<Key>, allocator< pair<const Key,Value> > >;

   typedef bip::basic_string< char, std::char_traits< char >, allocator< char > > shared_string;


   template<typename Object, typename... Args>
   using shared_multi_index_container = boost::multi_index_container<Object,Args..., eosio::blockchain::allocator<Object> >;


   typedef uint64_t scope_name;
   typedef uint64_t table_name;



   /**
    *  Object ID type that includes the type of the object it references
    */
   template<typename T>
   struct oid {
      oid( int64_t i = 0 ):_id(i){}

      oid& operator++() { ++_id; return *this; }

      friend bool operator < ( const oid& a, const oid& b ) { return a._id < b._id; }
      friend bool operator > ( const oid& a, const oid& b ) { return a._id > b._id; }
      friend bool operator == ( const oid& a, const oid& b ) { return a._id == b._id; }
      friend bool operator != ( const oid& a, const oid& b ) { return a._id != b._id; }
      friend std::ostream& operator<<(std::ostream& s, const oid& id) {
         s << boost::core::demangle(typeid(oid<T>).name()) << '(' << id._id << ')'; return s;
      }

      int64_t _id = 0;
   };


   template<uint16_t TypeNumber, typename Derived>
   struct object {
      typedef oid<Derived> id_type;
      enum type_id_enum {
        type_id = TypeNumber
      };
   };

   /** this class is ment to be specified to enable lookup of index type by object type using
    * the SET_INDEX_TYPE macro.
    **/
   template<typename T>
   struct get_index_type {};

   /**
    *  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
    */
   #define EOSIO_SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )  \
   namespace eosio { namespace blockchain { template<> struct get_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; }; } }

   #define EOSIO_DEFAULT_CONSTRUCTOR( OBJECT_TYPE ) \
   template<typename Constructor, typename Allocator> \
   OBJECT_TYPE( Constructor&& c, Allocator&&  ) { c(*this); }


   /**
    * This backups up the state of generic_table so that it can be restored after any
    * edits are made.
    */
   template< typename ValueType >
   struct table_undo_state
   {
      typedef typename ValueType::id_type                      id_type;
      typedef allocator< std::pair<const id_type, ValueType> > id_value_allocator_type;
      typedef allocator< id_type >                             id_allocator_type;

      template<typename T>
      table_undo_state( allocator<T> al )
      :old_values( id_value_allocator_type( al.get_segment_manager() ) ),
       removed_values( id_value_allocator_type( al.get_segment_manager() ) ),
       new_ids( id_allocator_type( al.get_segment_manager() ) ){}

      typedef shared_map<id_type,ValueType>  id_value_type_map;
      typedef shared_set< id_type >          id_type_set;

      id_value_type_map            old_values;
      id_value_type_map            removed_values;
      id_type_set                  new_ids;
      id_type                      old_next_id = 0;

      /**
       *  The revision the table will be in after restoring old_values, removed_values, etc.
       */
      int64_t                      revision = 0;
   };

   struct scope_state;

   /**
    *  Common base class for all tables, it includes the type of the data stored so
    *  that the proper virtual interface can be looked up for dealing with the true
    *  table.
    */
   struct table {
      offset_ptr<const scope_state> _scope; /// weak reference, table is owned by scope
      uint64_t                      _type = 0;
      uint64_t                      _name = 0;
   };

   /**
    *  The purpose of this class is to wrap tables that are returned to ensure
    *  that all updates to the table get tracked in the shard_undo_state
    */
   template<typename MultiIndexType>
   class table_handle
   {
      public:
        template<typename Constructor>
        const value_type& emplace( Constructor&& c ) {
           _suds.on_modify( _table );
           return _table.emplace( std::forward<Constructor>( c ) );
        }

        /**
         * Make read-only access to the underlying table as easy as using a pointer operator
         */
        const generic_table<MultiIndexType>& operator->()const { return _table; }

      private:
         table_handle( generic_table<MultiIndexType>& table, shard_undo_state& suds )
         :_table(table),_suds(suds){}

         generic_table<MultiIndexType>& _table;
         shard_undo_state&              _suds;
   };


   struct shard_undo_state {
      shard_undo_state( allocator<char> a ):new_tables(a),modified_tables(a),write_scopes(a){}

      void on_modify( table& t ) {
         auto itr = modified_tables.find( t._name );
         if( itr == modified_tables.end() )
            modified_tables.emplace( pair<table_name,offset_ptr<table>>(t._name,&t) );
      }

      offset_ptr<database_undo_state>              db_undo_state;   ///< points to the higher level undo state (where revision lives)
      shared_deque< pair<scope_name,table_name> >  new_tables;      ///< any time a new table is created it gets loged here so it can be removed if unwinding 
      shared_map<table_name, offset_ptr<table> >   modified_tables; ///< tracks the tables which have been modified so they can be undone 
      shared_vector<scope_name>                    write_scopes;
   };


   /**
    * Track changes to the database, at the start of each block/cycle we create a new
    * undo_state and set the increment the revision. This undo state can only be modified by
    * the main database thread; however, it breaks up the undo states for threads based upon
    * groups of scopes. 
    *
    * Given a scope we can find the current thread undo states.  Creating / modifying tables
    * will only impact thre shard_undo_state and therefore can occur in parallel across non
    * overlapping scopes.
    */
   struct database_undo_state {
      database_undo_state( allocator<char> a ):new_scopes(a),shard_undo_states(a),read_locks(a),threads(a){}

      uint64_t                                                 revision = 0; 
      shared_deque< offset_ptr<scope_state> >                  new_scopes;
      shared_map<scope_name, offset_ptr<shard_undo_state> >   shard_undo_states;
      shared_set<scope_name>                                   read_locks;
      shared_list<shard_undo_state>                           threads;
   };



   /**
    *  A generic_table wraps a boost multi_index container and provides version control along
    *  with a primary key type. 
    */
   template<typename MultiIndexType>
   struct generic_table : public table {
        typedef bip::managed_mapped_file::segment_manager             segment_manager_type;
        typedef MultiIndexType                                        index_type;
        typedef typename index_type::value_type                       value_type;
        typedef bip::allocator< generic_table, segment_manager_type > allocator_type;
        typedef table_undo_state< value_type >                        undo_state_type;
        typedef typename value_type::id_type                          id_type;
        
        
        template<typename Allocator>
        generic_table( Allocator&& a )
        :_stack(a),_indices(a) {
           table::_type = value_type::type_id;
           ilog( "generic_table constructor" );
        }
        ~generic_table() {
           ilog( "~generic_table destructor" );
        }
        
        /**
         * Construct a new element in the multi_index_container.
         * Set the ID to the next available ID, then increment _next_id and fire off on_create().
         */
        template<typename Constructor>
        const value_type& emplace( Constructor&& c ) {
           auto new_id = _next_id;
        
           auto constructor = [&]( value_type& v ) {
              v.id = new_id;
              c( v );
           };
        
           auto insert_result = _indices.emplace( constructor, _indices.get_allocator() );
        
           if( !insert_result.second ) {
              BOOST_THROW_EXCEPTION( std::logic_error("could not insert object, most likely a uniqueness constraint was violated") );
           }
        
           ++_next_id;
           on_create( *insert_result.first );
           return *insert_result.first;
        }

         void remove( const value_type& obj ) {
            on_remove( obj );
            _indices.erase( _indices.iterator_to( obj ) );
         }

         template<typename Modifier>
         void modify( const value_type& obj, Modifier&& m ) {
            on_modify( obj );
            auto ok = _indices.modify( _indices.iterator_to( obj ), m );
            if( !ok ) BOOST_THROW_EXCEPTION( std::logic_error( "Could not modify object, most likely a uniqueness constraint was violated" ) );
         }

        /**
         * Discards all undo history prior to revision
         */
        void commit( int64_t revision ) {
           while( _stack.size() && _stack[0].revision <= revision )
           {
              _stack.pop_front();
           }
        }

        /**
         * Unwinds all undo states to the last commited revision
         */
        void undo_all() {
           while( enabled() )
              undo();
        }

        /**
         *  This method works similar to git squash, it merges the change set from the two most
         *  recent revision numbers into one revision number (reducing the head revision number)
         *
         *  This method does not change the state of the index, only the state of the undo buffer.
         */
        void squash() {
           if( !enabled() ) return;
           if( _stack.size() == 1 ) {
              _stack.pop_front();
              return;
           }

           auto& state = _stack.back();
           auto& prev_state = _stack[_stack.size()-2];

           // An object's relationship to a state can be:
           // in new_ids            : new
           // in old_values (was=X) : upd(was=X)
           // in removed (was=X)    : del(was=X)
           // not in any of above   : nop
           //
           // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
           //
           //                   |--------------------- B ----------------------|
           //
           //                +------------+------------+------------+------------+
           //                | new        | upd(was=Y) | del(was=Y) | nop        |
           //   +------------+------------+------------+------------+------------+
           // / | new        | N/A        | new       A| nop       C| new       A|
           // | +------------+------------+------------+------------+------------+
           // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
           // A +------------+------------+------------+------------+------------+
           // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
           // | +------------+------------+------------+------------+------------+
           // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
           //   +------------+------------+------------+------------+------------+
           //
           // Each entry was composed by labelling what should occur in the given case.
           //
           // Type A means the composition of states contains the same entry as the first of the two merged states for that object.
           // Type B means the composition of states contains the same entry as the second of the two merged states for that object.
           // Type C means the composition of states contains an entry different from either of the merged states for that object.
           // Type N/A means the composition of states violates causal timing.
           // Type AB means both type A and type B simultaneously.
           //
           // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the composition of
           // state A and B.
           //
           // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged state.
           // Type B (and AB) can be implemented by copying from state to prev_state.
           // Type C needs special case-by-case logic.
           // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
           // (a serious logic error which should never happen).
           //

           // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three containers.

           for( const auto& item : state.old_values )
           {
              if( prev_state.new_ids.find( item.second.id ) != prev_state.new_ids.end() )
              {
                 // new+upd -> new, type A
                 continue;
              }
              if( prev_state.old_values.find( item.second.id ) != prev_state.old_values.end() )
              {
                 // upd(was=X) + upd(was=Y) -> upd(was=X), type A
                 continue;
              }
              // del+upd -> N/A
              assert( prev_state.removed_values.find(item.second.id) == prev_state.removed_values.end() );
              // nop+upd(was=Y) -> upd(was=Y), type B
              prev_state.old_values.emplace( std::move(item) );
           }

           // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
           for( auto id : state.new_ids )
              prev_state.new_ids.insert(id);

           // *+del
           for( auto& obj : state.removed_values )
           {
              if( prev_state.new_ids.find(obj.second.id) != prev_state.new_ids.end() )
              {
                 // new + del -> nop (type C)
                 prev_state.new_ids.erase(obj.second.id);
                 continue;
              }
              auto it = prev_state.old_values.find(obj.second.id);
              if( it != prev_state.old_values.end() )
              {
                 // upd(was=X) + del(was=Y) -> del(was=X)
                 prev_state.removed_values.emplace( std::move(*it) );
                 prev_state.old_values.erase(obj.second.id);
                 continue;
              }
              // del + del -> N/A
              assert( prev_state.removed_values.find( obj.second.id ) == prev_state.removed_values.end() );
              // nop + del(was=Y) -> del(was=Y)
              prev_state.removed_values.emplace( std::move(obj) ); //[obj.second->id] = std::move(obj.second);
           }

           _stack.pop_back();
        }

        /**
         *  Revert table until its state is identical to the state it was at the start of
         *  @param revision.  
         *
         *  Calls undo() while the current revision is greater than or
         *  equal to revision.
         */
        void undo_until( uint64_t revision ) {
           const auto& head = _stack.back();
           while( head.revision >= revision )
              undo();
        }

        /**
         *  Restores the state to how it was prior to the current session discarding all changes
         *  made between the last revision and the current revision.
         */
        void undo() {
           if( !enabled() ) return;

           const auto& head = _stack.back();

           for( auto& item : head.old_values ) {
              auto ok = _indices.modify( _indices.find( item.second.id ), [&]( value_type& v ) {
                 v = std::move( item.second );
              });
              if( !ok ) BOOST_THROW_EXCEPTION( std::logic_error( "Could not modify object, most likely a uniqueness constraint was violated" ) );
           }

           for( auto id : head.new_ids )
           {
              _indices.erase( _indices.find( id ) );
           }
           _next_id = head.old_next_id;

           for( auto& item : head.removed_values ) {
              bool ok = _indices.emplace( std::move( item.second ) ).second;
              if( !ok ) BOOST_THROW_EXCEPTION( std::logic_error( "Could not restore object, most likely a uniqueness constraint was violated" ) );
           }

           _stack.pop_back();
        }

      private:
        void on_create( const value_type& v ) {
           if( !enabled() ) return;
           auto& head = _stack.back();
        
           head.new_ids.insert( v.id );
        }
        void on_remove( const value_type& v ) {
           if( !enabled() ) return;

           auto& head = _stack.back();
           if( head.new_ids.count(v.id) ) {
              head.new_ids.erase( v.id );
              return;
           }

           auto itr = head.old_values.find( v.id );
           if( itr != head.old_values.end() ) {
              head.removed_values.emplace( std::move( *itr ) );
              head.old_values.erase( v.id );
              return;
           }

           if( head.removed_values.count( v.id ) )
              return;

           head.removed_values.emplace( std::pair< typename value_type::id_type, const value_type& >( v.id, v ) );
        }
         void on_modify( const value_type& v ) {
            if( !enabled() ) return;

            auto& head = _stack.back();

            if( head.new_ids.find( v.id ) != head.new_ids.end() )
               return;

            auto itr = head.old_values.find( v.id );
            if( itr != head.old_values.end() )
               return;

            head.old_values.emplace( std::pair< typename value_type::id_type, const value_type& >( v.id, v ) );
         }
        
        bool enabled()const { return _stack.size(); }
        shared_deque< undo_state_type > _stack;
        index_type                      _indices;
        id_type                         _next_id;
   };

   /*
   struct session {
      shared_vector< offset_ptr<table> > modified_tables;
   };

   struct cycle {
      cycle( allocator<char> a )
      :_write_locks(a), _read_locks(a), _sessions(a){}
      shared_set<scope_name>            _write_locks;
      shared_set<scope_name>            _read_locks;
      shared_vector< session >          _sessions;
   };
   */

   
   struct scope_state {
      template<typename Allocator>
      scope_state( Allocator&& a ):_tables( std::forward<Allocator>(a) ){}

      ~scope_state() {
         idump((_name));
      }

      const table& get_table( table_name name )const {
         auto itr = _tables.find( name );
         FC_ASSERT( itr != _tables.end(), "unknown table" );
         return *itr->second;
      }

      scope_name                                  _name;
      shared_map< table_name, offset_ptr<table> > _tables;
      //offset_ptr<database::shard_undo_state>     _current_undo_state;
   };

   /*
   struct scope_pair {
      scope_name              scope;
      offset_ptr<scope_state> state;
   };

   struct by_scope;
   shared_multi_index_container< scope_pair,
      indexed_by<
         ordered_unique< tag<by_scope>, member<scope_pair, scope_name, &scope_state::scope_name> >
      >
   >
   */

   struct abstract_table {
      virtual void destruct( table* t, allocator<char> a ) = 0;
      virtual void undo_until( table& t, uint64_t revision ) = 0;
   };

   template<typename T>
   struct table_interface_impl : public abstract_table {
      virtual void destruct( table* t, allocator<char> a ) override {
         a.get_segment_manager()->destroy_ptr(static_cast<T*>(t));
      }
      virtual void undo_until( table& t, uint64_t revision ) override {
         static_cast<T&>(t).undo_until( revision );
      }
   };

   /**
    *  @class database
    *  @brief high level API for memory mapped database
    *
    *  This database organizes data into named scopes and tables with the tables
    *  being implemented as boost multi_index containers. Furthermore, this database
    *  adds support for revision tracking so that changes can be reliably undone in
    *  the event of an error.
    *
    *  Lastly, this database is designed to enable changes to be made by multiple threads
    *  at the same time so long as each thread operates in non-intersecting scope groups.
    */
   class database 
   {
      public:
         database( const path& dir, uint64_t shared_file_size = 0 );
         ~database();

         void flush();

         /// register virtual API (construct, destruct, insert, modify, etc)
         template<typename ObjectType>
         void register_table() {
            typedef typename get_index_type<ObjectType>::type index_type;
            typedef generic_table<index_type> generic_object_table_type;
            FC_ASSERT( !is_registered_table<ObjectType>(), "Table type already registered" );
            _table_interfaces[ObjectType::type_id] = new table_interface_impl<generic_object_table_type>();
         }

         uint64_t revision()const;

         /** If there is non undo history then the initial revision number can be set to 
          * anything we want. This is normally called on startup after calling undo_all()
          */
         void set_revision( uint64_t revision );
         void push_revision( uint64_t revision );
         void pop_revision( uint64_t revision );

         void undo_all();
         void undo();

         session&  start_session( const flat_set<uint64_t>& scopes, bool undo_enabled );

         const scope_state* find_scope( scope_name scope )const;
         const scope_state& get_scope( scope_name scope )const;
         const scope_state& create_scope( scope_name scope );

         class thread {
            public:
               template<typename ObjectType>
               auto& create_table( const scope_state& scope, table_name tbl )// -> const generic_table<typename get_index_type<ObjectType>::type>& 
               {
                  /// TODO: verify scope
                  return _db.create_table<ObjectType>( _suds, scope, tbl );
               }

               void delete_table( const table& tbl ) {
                  /// TODO: verify scope
                  _db.delete_table( tbl );
               }

               template<typename ObjectType, typename Modifier>
               const auto& modify( const ObjectType& obj, Modifier&& m ) {
                  _db.modify( _suds, obj, std::forward<Modifier>(m) );
               }

            private:
               friend class database;
               thread( database& db, database::shard_undo_state& suds ):_db(db),_suds(suds){}
               thread( const thread& ) = delete;
               thread( thread&& mv ):_db(mv._db),_suds(mv._suds){}

               database&             _db;
               shard_undo_state&    _suds;
         };


         /**
          * Starts a new shard undo state for the following scopes, if a shard already exists
          * for any of the scopes then this method will fail.
          */
         shard start_shard( const flat_set<scope_name>& write_scopes, const flat_set<scope_name>& read_scopes = flat_set<scope_name>()  );

      private:
         friend class thread;
         void delete_table( const table& tbl );
         void delete_scope( scope_state& scope );

         scope_state* find_scope( scope_name scope );
         scope_state& get_scope( scope_name scope );


         template<typename ObjectType>
         auto create_table( shard_undo_state& suds, const scope_state& scope, table_name tbl ) -> const generic_table<typename get_index_type<ObjectType>::type>& 
         {
            scope_state& mscope = const_cast<scope_state&>(scope);

            typedef typename get_index_type<ObjectType>::type index_type;
            typedef generic_table<index_type> generic_object_table_type;
            FC_ASSERT( is_registered_table<ObjectType>(), "unknown table type being registered" );

            auto memory = _segment->get_segment_manager();

            auto& tables = mscope._tables;

            auto table_itr = tables.find( tbl );
            FC_ASSERT( table_itr == tables.end(), "table already exists" );

            table* new_table = static_cast<table*>(_segment->construct<generic_object_table_type>( bip::anonymous_instance )( memory ) );

            new_table->_name = tbl;
            new_table->_scope = &scope;
            ilog( "create table" );
            table_itr = tables.emplace( tbl, new_table ).first;

            suds.new_tables.emplace_front( pair<scope_name,table_name>(scope._name, tbl) );
            return static_cast</*const*/ generic_object_table_type&>(*table_itr->second);
         }

         template<typename ObjectType, typename Modifier>
         const auto& modify( shard_undo_state& suds, const ObjectType& obj, Modifier&& m ) {

            return obj;
         }





         shard_undo_state* find_shard_undo_state_for_scope( scope_name scope )const;

         template<typename ObjectType>
         bool is_registered_table()const {
            auto itr = _table_interfaces.find( ObjectType::type_id );
            return itr != _table_interfaces.end();
         }


         abstract_table& get_interface( const table& t )const {
            return get_interface( t._type );
         }

         abstract_table& get_interface( uint16_t table_type )const {
            auto itr = _table_interfaces.find(table_type);
            FC_ASSERT( itr != _table_interfaces.end(), "unknown table type" );
            return *itr->second;
         }

         typedef shared_map< scope_name, bip::offset_ptr<scope_state> > scopes_type;
         map<uint16_t, abstract_table*>        _table_interfaces;

         bfs::path                             _data_dir;

         bip::file_lock                        _flock;
         unique_ptr<bip::managed_mapped_file>  _segment;
         unique_ptr<bip::managed_mapped_file>  _meta;

         /// Variables below this point are allocated in shared memory 
         scopes_type*                          _scopes = nullptr; /// allocated in shared memory
         shared_deque< database_undo_state >*  _undo_stack = nullptr;
   };

} } /// eosio::blockchain
