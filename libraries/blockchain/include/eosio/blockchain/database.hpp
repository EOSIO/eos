#pragma once
#include <eosio/database/generic_table.hpp>

namespace eosio { namespace blockchain {


   typedef uint64_t scope_name;
   typedef uint64_t table_name;







   struct scope_state;



   struct shard_undo_state;

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
      database_undo_state( allocator<char> a ):new_scopes(a),shard_undo_states(a),read_locks(a),shards(a){}

      uint64_t                                                 revision = 0; 
      shared_deque< offset_ptr<scope_state> >                  new_scopes;
      shared_map<scope_name, offset_ptr<shard_undo_state> >    shard_undo_states;
      shared_set<scope_name>                                   read_locks;
      shared_list<shard_undo_state>                            shards;
   };

   struct shard_undo_state {
      shard_undo_state( allocator<char> a ):new_tables(a),modified_tables(a),write_scopes(a){}

      void on_modify( table& t ) {
         auto itr = modified_tables.find( t._name );
         if( itr == modified_tables.end() )
            modified_tables.emplace( pair<table_name,offset_ptr<table>>(t._name,&t) );
      }

      int64_t revision()const { return db_undo_state->revision; }

      offset_ptr<database_undo_state>              db_undo_state;   ///< points to the higher level undo state (where revision lives)
      shared_deque< pair<scope_name,table_name> >  new_tables;      ///< any time a new table is created it gets loged here so it can be removed if unwinding 
      shared_map<table_name, offset_ptr<table> >   modified_tables; ///< tracks the tables which have been modified so they can be undone 
      shared_vector<scope_name>                    write_scopes;
   };




   struct scope_state {
      template<typename Allocator>
      scope_state( Allocator&& a ):_tables( std::forward<Allocator>(a) ){}

      ~scope_state() {
         idump((_name));
      }

      const table* find_table( table_name name )const {
         idump((name));
         auto itr = _tables.find( name );
         return itr == _tables.end() ? nullptr : &*itr->second;
      }

      table* find_table( table_name name ) {
         idump((name));
         auto itr = _tables.find( name );
         return itr == _tables.end() ? nullptr : &*itr->second;
      }

      const table& get_table( table_name name )const {
         auto t = find_table(name);
         FC_ASSERT( t != nullptr, "unknown table" );
         return *t;
      }

      scope_name                                  _name;
      shared_map< table_name, offset_ptr<table> > _tables;
      //offset_ptr<database::shard_undo_state>     _current_undo_state;
   };



   class database;

   template<typename ObjectType>
   class table_handle;

   class shard {
      public:
         template<typename ObjectType>
         table_handle<ObjectType> create_table( scope_name, table_name tbl );

         template<typename ObjectType>
         optional<table_handle<ObjectType>> find_table( scope_name, table_name tbl );

         void delete_table( const table& tbl ); 

         template<typename ObjectType, typename Modifier>
         const auto& modify( const ObjectType& obj, Modifier&& m );

         shard( shard&& mv ):_db(mv._db),_suds(mv._suds){}
      private:
         friend class database;
         shard( database& db, shard_undo_state& suds ):_db(db),_suds(suds){}
         shard( const shard& ) = delete;

         database&             _db;
         shard_undo_state&    _suds;
   };

   class session {
      public:
         session( session&& mv );
         ~session();

         void undo();
         void push();

         shard start_shard( const flat_set<scope_name>& scopes, const flat_set<scope_name>& read_scopes = flat_set<scope_name>() );
         void create_scope( scope_name scope );

      private:
         friend class database;
         session( database& db, database_undo_state& duds );

         database&            _db;
         database_undo_state& _duds;
         bool                 _undo = true;
   };


   /**
    *  The purpose of this class is to wrap tables that are returned to ensure
    *  that all updates to the table get tracked in the shard_undo_state
    */
   template<typename ObjectType>
   class table_handle
   {
      public:
        typedef typename get_index_type<ObjectType>::type multi_index_type;

        template<typename Constructor>
        const auto& emplace( Constructor&& c );

        template<typename Modifier>
        void modify( const ObjectType& o, Modifier&& c );

        void remove( const ObjectType& o );

        // Make read-only access to the underlying table as easy as using a pointer operator
        const generic_table<multi_index_type>& operator->()const { return _table; }

      private:
         friend class shard;
         table_handle( scope_state& scope, generic_table<multi_index_type>& table, shard_undo_state& suds )
         :_scope(scope),_table(table),_suds(suds){}

         scope_state&                       _scope;
         generic_table<multi_index_type>&   _table;
         shard_undo_state&                  _suds;
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
    *  at the same time so long as each thread operates in non-intersecting scope groups (shards).
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
         session  start_session( uint64_t revision );

         /** If there is non undo history then the initial revision number can be set to 
          * anything we want. This is normally called on startup after calling undo_all()
          */
         void set_revision( uint64_t revision );
         void push_revision( uint64_t revision );
         void pop_revision( uint64_t revision );
         void commit_revision( uint64_t revision );

         void undo_all();
         void undo();

         const scope_state* find_scope( scope_name scope )const;
         const scope_state& get_scope( scope_name scope )const;



      private:
         friend class shard;
         friend class session;

         /**
          * Starts a new shard undo state for the following scopes, if a shard already exists
          * for any of the scopes then this method will fail.
          */
         shard start_shard( database_undo_state& duds, const flat_set<scope_name>& write_scopes, const flat_set<scope_name>& read_scopes = flat_set<scope_name>()  );
         void create_scope( database_undo_state& duds, scope_name scope );

         void delete_table( const table& tbl );
         void delete_scope( scope_state& scope );

         scope_state* find_scope( scope_name scope );
         scope_state& get_scope( scope_name scope );


         template<typename ObjectType>
         auto find_table( shard_undo_state& suds, const scope_state& scope, table_name tbl ) {
            scope_state& mscope = const_cast<scope_state&>(scope);

            typedef typename get_index_type<ObjectType>::type index_type;
            typedef generic_table<index_type> generic_object_table_type;
            FC_ASSERT( is_registered_table<ObjectType>(), "unknown table type being registered" );

            generic_object_table_type* result = nullptr;

            auto t = mscope.find_table(tbl);

            if( t && t->_type == generic_object_table_type::value_type::type_id )
               result = static_cast<generic_object_table_type*>(t);
            return result;
         } // find_table


         template<typename ObjectType>
         auto& create_table( shard_undo_state& suds, const scope_state& scope, table_name tbl )
         {
            scope_state& mscope = const_cast<scope_state&>(scope);

            typedef typename get_index_type<ObjectType>::type index_type;
            typedef generic_table<index_type> generic_object_table_type;
            FC_ASSERT( is_registered_table<ObjectType>(), "unknown table type being registered" );

            auto memory = _segment->get_segment_manager();

            auto& tables = mscope._tables;

            auto table_itr = tables.find( tbl );
            FC_ASSERT( table_itr == tables.end(), "table already exists" );

            auto gtable = _segment->construct<generic_object_table_type>( bip::anonymous_instance )( memory );
            gtable->start_revision( suds.revision() );
            table* new_table = static_cast<table*>(gtable);

            new_table->_name = tbl;
            new_table->_scope = &scope;
            ilog( "create table ${t}", ("t",tbl) );
            table_itr = tables.emplace( tbl, new_table ).first;

            suds.new_tables.emplace_front( pair<scope_name,table_name>(scope._name, tbl) );
            return static_cast<generic_object_table_type&>(*table_itr->second);
         } // create_table

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


   
   template<typename ObjectType, typename Modifier>
   const auto& shard::modify( const ObjectType& obj, Modifier&& m ) {
      _db.modify( _suds, obj, std::forward<Modifier>(m) );
   }


   template<typename ObjectType>
   optional<table_handle<ObjectType>> shard::find_table( scope_name s, table_name t ) {
      auto& scoperef = _db.get_scope(s);
      auto* tableref = _db.find_table<ObjectType>( _suds, scoperef, t );
      wdump((int64_t(tableref)));
      if( tableref )
         return table_handle<ObjectType>( scoperef, *tableref, _suds );
      return optional<table_handle<ObjectType>>();
   }

   template<typename ObjectType>
   table_handle<ObjectType> shard::create_table( scope_name s, table_name t )
   {
      typedef typename get_index_type<ObjectType>::type multi_index_type;

      auto& scoperef = _db.get_scope(s);
      //auto& tableref = scoperef.get_table(t);
      auto& tableref = _db.create_table<ObjectType>( _suds, scoperef, t );
      //FC_ASSERT( tableref.type == multi_index_type::id_type, "table type mistmatch" );
      return table_handle<ObjectType>( scoperef, static_cast<generic_table<multi_index_type>&>(tableref), _suds );
   }


   template<typename ObjectType>
   template<typename Constructor>
   const auto& table_handle<ObjectType>::emplace( Constructor&& c ) {
      _suds.on_modify( _table );
      return _table.emplace( std::forward<Constructor>( c ) );
   }

   template<typename ObjectType>
   template<typename Modifier>
   void table_handle<ObjectType>::modify( const ObjectType& obj, Modifier&& m ) {
      _suds.on_modify( _table );
      _table.modify( obj, std::forward<Modifier>( m ) );
   }

   template<typename ObjectType>
   void table_handle<ObjectType>::remove( const ObjectType& obj ) {
      _suds.on_modify( _table );
      _table.remove( obj );
   }


} } /// eosio::blockchain
