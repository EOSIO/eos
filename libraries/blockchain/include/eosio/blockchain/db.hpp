#pragma once
#include <eosio/blockchain/generic_table.hpp>

namespace eosio { namespace blockchain  {

   typedef uint64_t scope_name;
   typedef uint64_t table_name;
   typedef uint64_t block_num_type;
   typedef uint16_t table_id_type;

   struct shard;

   template<typename ObjectType> 
   class table_handle;
   class block_handle;
   class scope_handle;
   class transaction_handle;
   class cycle_handle;
   class shard_handle;
   class database;

   using std::set;
   using std::deque;
   using std::vector;


   struct undo_transaction {
      undo_transaction( allocator<char> a, shard& s )
      :new_scopes(a),new_tables(a),modified_tables(a),_shard(&s){}

      shared_vector<scope_name>     new_scopes;
      shared_set<table_optr>        new_tables;
      shared_set<table_optr>        modified_tables;
      offset_ptr<undo_transaction>  next;
      offset_ptr<shard>             _shard;
   };

   struct shard {
      shard( allocator<char> a, const set<scope_name>& w, const set<scope_name>& r );

      shared_set<scope_name>        _write_scope;
      shared_set<scope_name>        _read_scope;
      shared_list<undo_transaction> _transaction_history;
   };

   typedef offset_ptr<shard> shard_optr;

   struct undo_cycle {
      undo_cycle( allocator<char> a ):_write_scope_to_shard(a),_read_scope_to_shard(a),_shards(a){}

      shared_map<scope_name,shard_optr> _write_scope_to_shard;
      shared_map<scope_name,shard_optr> _read_scope_to_shard;
      shared_list<shard>                _shards;
   };

   struct undo_block{
      undo_block( allocator<char> a ):_cycle_undo_history(a){}

      block_num_type           _block_num = 0;
      shared_deque<undo_cycle> _cycle_undo_history;
   };

   struct scope {
      scope( allocator<char> a ):_tables(a){}
      ~scope() { assert( _tables.size() == 0 ); }

      const table* find_table( table_name t )const;

      shared_map<table_name,table_optr> _tables;
   };

   typedef offset_ptr<scope> scope_optr;



   template<typename IndexType> 
   class table_handle {
      public:
         table_handle( generic_table<IndexType>& t ) 
         :_table( t )
         {
         }
      private:
         generic_table<IndexType>& _table;
   };


   class scope_handle {
      public:
         scope_handle( transaction_handle& trx, scope& s );

         template<typename ObjectType> 
         auto create_table( table_name t );

         template<typename ObjectType> 
         optional<table_handle<ObjectType>> find_table( table_name t );

         template<typename ObjectType> 
         table_handle<ObjectType>           get_table( table_name t );
      private:
         scope&                _scope;
         transaction_handle&   _trx;
   };

   class transaction_handle
   {
      public:
         transaction_handle( shard_handle& sh, undo_transaction& udt );
         transaction_handle( transaction_handle&& mv );
         ~transaction_handle() {
            if( !_applied ) undo();
         }
         void squash() { _applied = true; }
         void undo();
         void commit() { _applied = true; }


         /**
          * This only works if this transaction_handle is the head transaction handle
          * and we have 'scope' scope. We cannot have two transactions in different shards
          * creating new scopes, but we need scopes to be undoable as part of transactions.
          *
          * The scope handle is only good for the scope of the transaction
          */
         scope_handle create_scope( scope_name s );

      private:
         friend class scope_handle;

         void on_create_table( table& t );

         template<typename ObjectType>
         void on_modify_table( generic_table<ObjectType>& t );

         bool              _applied = false;
         database&         _db;
         undo_transaction& _undo_trx;
   };


   class shard_handle {
      public:
         shard_handle( cycle_handle& c, shard& s );

         transaction_handle start_transaction();

      private:
         friend class transaction_handle;

         void undo_transaction();
         void squash_transaction();

         database& _db;
         shard&    _shard;
   };


   class cycle_handle {
      public:
         cycle_handle( const block_handle& blk, undo_cycle& cyc );

         /**
          * Assuming this block is the current head block handle, this will attempt to
          * create a new shard.  It will fail if there is a conflict with the read/write scopes and
          * existing shards.
          */
         shard_handle start_shard( const set<scope_name>& write, const set<scope_name>& read = set<scope_name>() );
      private:
         friend class shard_handle;
         database&     _db;
         undo_block&   _undo_block;
         undo_cycle&   _undo_cycle;
   };

   class block_handle {
      public:

         block_handle( block_handle&& mv )
         :_db(mv._db),_undo_block(mv._undo_block) {
            _undo = mv._undo;
            mv._undo = false;
         }

         block_handle( database& db, undo_block& udb )
         :_db(db),_undo_block(udb){}

         ~block_handle() {
            if( _undo ) undo();
         }

         cycle_handle start_cycle();

         void commit() { _undo = false; }
         void undo();

      private:
         friend class cycle_handle;
         bool        _undo = true;
         database&   _db;
         undo_block& _undo_block;
   };


   /**
    *  @class database
    *  @brief a revision tracking memory mapped database
    *
    *  Maintains a set of scopes which contain a set of generic_table. Changes to the database
    *  are tracked so that they can be rolled back in the event of an error. Access to the database
    *  is safe to perform in parallel so long as new scopes are only created in shards with "scope" scope.
    */
   class database {
      public:
         static const block_num_type max_block_num = -1;


         database( const path& dir, uint64_t shared_file_size = 0 );
         ~database();

         /** the last block whose changes we can revert */
         block_num_type last_reversible_block()const;

         /** starts a new block */
         block_handle start_block( block_num_type block = 1 );
         block_handle head_block();

         /**
          * Undoes the changes made since the most recent call to start_block()
          */
         void undo();

         /** Undoes all changes made in blocks greater than blocknum */
         void undo_until( block_num_type blocknum );

         /** marks all blocks before and including blocknum irreversible */
         void commit( block_num_type blocknum );

         allocator<char> get_allocator() { return _segment->get_segment_manager(); }

         const scope* find_scope( scope_name n )const;
         const scope& get_scope( scope_name n )const;

      private:
         friend class transaction_handle;
         friend class block_handle;
         friend class scope_handle;

         void undo( undo_block& udb );
         void undo( undo_transaction& udb );

         scope* create_scope( scope_name n );
         void   delete_scope( scope_name n );

         template<typename T>
         T* construct() {
            auto memory = _segment->get_segment_manager();
            return _segment->construct<T>( bip::anonymous_instance )( memory );
         }

         bfs::path                             _data_dir;
         bip::file_lock                        _flock;
         unique_ptr<bip::managed_mapped_file>  _segment;
         unique_ptr<bip::managed_mapped_file>  _meta;

         /// Variables below this point are allocated in shared memory 
         typedef shared_map< scope_name, scope > scopes_type;

         scopes_type*                          _scopes = nullptr; /// allocated in shared memory
         shared_deque<undo_block>*             _block_undo_history = nullptr;
   };


   /**
    *  Create a new table, informing the transaction undo state of that this table
    *  might be modified. If it is the first time this table was added to the transactions
    *  modified tables, then push an extra undo session to the generic table
    */
   template<typename ObjectType> 
   auto scope_handle::create_table( table_name t ) {
      typedef typename get_index_type<ObjectType>::type index_type;

      FC_ASSERT( is_registered_table<ObjectType>(), "unknown table type" );

      const table* existing_table = _scope.find_table(t);
      FC_ASSERT( !existing_table, "table with name ${n} already exists", ("n",name(t)) );


      wlog( "creating table ${n}", ("n",name(t)) );
      auto* new_table = _trx._db.construct< generic_table<index_type> >(); 
      new_table->_scope = &_scope;
      new_table->_name  = t;

      _trx.on_create_table( *new_table );
      new_table->push();
      
      return table_handle<index_type>( *new_table );
   }

   template<typename IndexType>
   void transaction_handle::on_modify_table( generic_table<IndexType>& t ) {
      auto result = _undo_trx.modified_tables.insert(&t);
      if( result.second ) {  /// insert successful 
         t.push(); /// start a new undo tracking session for this table
      }
   }


      /*
      FC_ASSERT( existing_table->_type == ObjectType::type_id, "mismatch of expected table types" );
      auto& gt = static_cast<generic_table<ObjectType>&>(*existing_table);
      */
   /*
   shard::squash() {

   }

   database::commit_block() {
      _block_undo_history.front().commit();
   }
   undo_block::commit() {
      for( auto& c : _cycle_undo_history )
         c.commit();
   }
   undo_cycle::commit() {
      for( auto& s : _shards ) 
         s.commit();
   }

   shard::commit() {
      for( auto& t : _transaction_history )
         t.commit();
   }

   undo_transaction::commit() {
      for( auto& t : modified_tables ) 
         t.commit();
   }

   table::commit() {
      undo.pop_front();
   }
   */

} } /// eosio::blockchain
