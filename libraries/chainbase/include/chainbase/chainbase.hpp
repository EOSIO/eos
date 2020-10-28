#pragma once

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/core/demangle.hpp>

#include <boost/multi_index_container.hpp>

#include <boost/chrono.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>

#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>

#include <chainbase/pinnable_mapped_file.hpp>
#include <chainbase/shared_cow_string.hpp>
#include <chainbase/chainbase_node_allocator.hpp>
#include <chainbase/undo_index.hpp>

#ifndef CHAINBASE_NUM_RW_LOCKS
   #define CHAINBASE_NUM_RW_LOCKS 10
#endif

#ifdef CHAINBASE_CHECK_LOCKING
   #define CHAINBASE_REQUIRE_READ_LOCK(m, t) require_read_lock(m, typeid(t).name())
   #define CHAINBASE_REQUIRE_WRITE_LOCK(m, t) require_write_lock(m, typeid(t).name())
#else
   #define CHAINBASE_REQUIRE_READ_LOCK(m, t)
   #define CHAINBASE_REQUIRE_WRITE_LOCK(m, t)
#endif

namespace chainbase {

   namespace bip = boost::interprocess;
   namespace bfs = boost::filesystem;
   using std::unique_ptr;
   using std::vector;

   template<typename T>
   using allocator = bip::allocator<T, pinnable_mapped_file::segment_manager>;

   template<typename T>
   using node_allocator = chainbase_node_allocator<T, pinnable_mapped_file::segment_manager>;

   using shared_string = shared_cow_string;

   typedef boost::interprocess::interprocess_sharable_mutex read_write_mutex;
   typedef boost::interprocess::sharable_lock< read_write_mutex > read_lock;

   /**
    *  Object ID type that includes the type of the object it references
    */
   template<typename T>
   class oid {
      public:
         oid( int64_t i = 0 ):_id(i){}

         oid& operator++() { ++_id; return *this; }

         friend bool operator < ( const oid& a, const oid& b ) { return a._id < b._id; }
         friend bool operator > ( const oid& a, const oid& b ) { return a._id > b._id; }
         friend bool operator <= ( const oid& a, const oid& b ) { return a._id <= b._id; }
         friend bool operator >= ( const oid& a, const oid& b ) { return a._id >= b._id; }
         friend bool operator == ( const oid& a, const oid& b ) { return a._id == b._id; }
         friend bool operator != ( const oid& a, const oid& b ) { return a._id != b._id; }
         friend std::ostream& operator<<(std::ostream& s, const oid& id) {
            s << boost::core::demangle(typeid(oid<T>).name()) << '(' << id._id << ')'; return s;
         }

         int64_t _id = 0;
   };

   template<uint16_t TypeNumber, typename Derived>
   struct object
   {
      typedef oid<Derived> id_type;
      static const uint16_t type_id = TypeNumber;
   };

   /** this class is ment to be specified to enable lookup of index type by object type using
    * the SET_INDEX_TYPE macro.
    **/
   template<typename T>
   struct get_index_type {};

   /**
    *  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
    */
   #define CHAINBASE_SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )  \
   namespace chainbase { template<> struct get_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; }; }

   #define CHAINBASE_DEFAULT_CONSTRUCTOR( OBJECT_TYPE ) \
   template<typename Constructor, typename Allocator> \
   OBJECT_TYPE( Constructor&& c, Allocator&&  ) { c(*this); }

   /**
    * The code we want to implement is this:
    *
    * ++target; try { ... } finally { --target }
    *
    * In C++ the only way to implement finally is to create a class
    * with a destructor, so that's what we do here.
    */
   class int_incrementer
   {
      public:
         int_incrementer( int32_t& target ) : _target(target)
         { ++_target; }
         ~int_incrementer()
         { --_target; }

         int32_t get()const
         { return _target; }

      private:
         int32_t& _target;
   };

   template<typename MultiIndexType>
   using generic_index = multi_index_to_undo_index<MultiIndexType>;

   class abstract_session {
      public:
         virtual ~abstract_session(){};
         virtual void push()             = 0;
         virtual void squash()           = 0;
         virtual void undo()             = 0;
   };

   template<typename SessionType>
   class session_impl : public abstract_session
   {
      public:
         session_impl( SessionType&& s ):_session( std::move( s ) ){}

         virtual void push() override  { _session.push();  }
         virtual void squash() override{ _session.squash(); }
         virtual void undo() override  { _session.undo();  }
      private:
         SessionType _session;
   };

   class abstract_index
   {
      public:
         abstract_index( void* i ):_idx_ptr(i){}
         virtual ~abstract_index(){}
         virtual void     set_revision( uint64_t revision ) = 0;
         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) = 0;

         virtual int64_t revision()const = 0;
         virtual void    undo()const = 0;
         virtual void    squash()const = 0;
         virtual void    commit( int64_t revision )const = 0;
         virtual void    undo_all()const = 0;
         virtual uint32_t type_id()const  = 0;
         virtual uint64_t row_count()const = 0;
         virtual const std::string& type_name()const = 0;
         virtual std::pair<int64_t, int64_t> undo_stack_revision_range()const = 0;

         virtual void remove_object( int64_t id ) = 0;

         void* get()const { return _idx_ptr; }
      private:
         void* _idx_ptr;
   };

   template<typename BaseIndex>
   class index_impl : public abstract_index {
      public:
         index_impl( BaseIndex& base ):abstract_index( &base ),_base(base){}

         virtual unique_ptr<abstract_session> start_undo_session( bool enabled ) override {
            return unique_ptr<abstract_session>(new session_impl<typename BaseIndex::session>( _base.start_undo_session( enabled ) ) );
         }

         virtual void     set_revision( uint64_t revision ) override { _base.set_revision( revision ); }
         virtual int64_t  revision()const  override { return _base.revision(); }
         virtual void     undo()const  override { _base.undo(); }
         virtual void     squash()const  override { _base.squash(); }
         virtual void     commit( int64_t revision )const  override { _base.commit(revision); }
         virtual void     undo_all() const override {_base.undo_all(); }
         virtual uint32_t type_id()const override { return BaseIndex::value_type::type_id; }
         virtual uint64_t row_count()const override { return _base.indices().size(); }
         virtual const std::string& type_name() const override { return BaseIndex_name; }
         virtual std::pair<int64_t, int64_t> undo_stack_revision_range()const override { return _base.undo_stack_revision_range(); }

         virtual void     remove_object( int64_t id ) override { return _base.remove_object( id ); }
      private:
         BaseIndex& _base;
         std::string BaseIndex_name = boost::core::demangle( typeid( typename BaseIndex::value_type ).name() );
   };

   template<typename IndexType>
   class index : public index_impl<IndexType> {
      public:
         index( IndexType& i ):index_impl<IndexType>( i ){}
   };


   class read_write_mutex_manager
   {
      public:
         read_write_mutex_manager()
         {
            _current_lock = 0;
         }

         ~read_write_mutex_manager(){}

         void next_lock()
         {
            _current_lock++;
            new( &_locks[ _current_lock % CHAINBASE_NUM_RW_LOCKS ] ) read_write_mutex();
         }

         read_write_mutex& current_lock()
         {
            return _locks[ _current_lock % CHAINBASE_NUM_RW_LOCKS ];
         }

         uint32_t current_lock_num()
         {
            return _current_lock;
         }

      private:
         std::array< read_write_mutex, CHAINBASE_NUM_RW_LOCKS >     _locks;
         std::atomic< uint32_t >                                    _current_lock;
   };


   /**
    *  This class
    */
   class database
   {
      public:
         enum open_flags {
            read_only     = 0,
            read_write    = 1
         };

         using database_index_row_count_multiset = std::multiset<std::pair<unsigned, std::string>>;

         database(const bfs::path& dir, open_flags write = read_only, uint64_t shared_file_size = 0, bool allow_dirty = false,
                  pinnable_mapped_file::map_mode = pinnable_mapped_file::map_mode::mapped);
         ~database();
         database(database&&) = default;
         database& operator=(database&&) = default;
         bool is_read_only() const { return _read_only; }
         void flush();
         void set_require_locking( bool enable_require_locking );

#ifdef CHAINBASE_CHECK_LOCKING
         void require_lock_fail( const char* method, const char* lock_type, const char* tname )const;

         void require_read_lock( const char* method, const char* tname )const
         {
            if( BOOST_UNLIKELY( _enable_require_locking & _read_only & (_read_lock_count <= 0) ) )
               require_lock_fail(method, "read", tname);
         }

         void require_write_lock( const char* method, const char* tname )
         {
            if( BOOST_UNLIKELY( _enable_require_locking & (_write_lock_count <= 0) ) )
               require_lock_fail(method, "write", tname);
         }
#endif

         struct session {
            public:
               session( session&& s ):_index_sessions( std::move(s._index_sessions) ){}
               session( vector<std::unique_ptr<abstract_session>>&& s ):_index_sessions( std::move(s) )
               {
               }

               ~session() {
                  undo();
               }

               void push()
               {
                  for( auto& i : _index_sessions ) i->push();
                  _index_sessions.clear();
               }

               void squash()
               {
                  for( auto& i : _index_sessions ) i->squash();
                  _index_sessions.clear();
               }

               void undo()
               {
                  for( auto& i : _index_sessions ) i->undo();
                  _index_sessions.clear();
               }

            private:
               friend class database;
               session(){}

               vector< std::unique_ptr<abstract_session> > _index_sessions;
         };

         session start_undo_session( bool enabled );

         int64_t revision()const {
             if( _index_list.size() == 0 ) return -1;
             return _index_list[0]->revision();
         }

         void undo();
         void squash();
         void commit( int64_t revision );
         void undo_all();


         void set_revision( uint64_t revision )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK( "set_revision", uint64_t );
             for( auto i : _index_list ) i->set_revision( revision );
         }


         template<typename MultiIndexType>
         void add_index() {
            const uint16_t type_id = generic_index<MultiIndexType>::value_type::type_id;
            typedef generic_index<MultiIndexType>          index_type;
            typedef typename index_type::allocator_type    index_alloc;

            std::string type_name = boost::core::demangle( typeid( typename index_type::value_type ).name() );

            if( !( _index_map.size() <= type_id || _index_map[ type_id ] == nullptr ) ) {
               BOOST_THROW_EXCEPTION( std::logic_error( type_name + "::type_id is already in use" ) );
            }

            index_type* idx_ptr = nullptr;
            if( _read_only )
               idx_ptr = _db_file.get_segment_manager()->find_no_lock< index_type >( type_name.c_str() ).first;
            else
               idx_ptr = _db_file.get_segment_manager()->find< index_type >( type_name.c_str() ).first;
            bool first_time_adding = false;
            if( !idx_ptr ) {
               if( _read_only ) {
                  BOOST_THROW_EXCEPTION( std::runtime_error( "unable to find index for " + type_name + " in read only database" ) );
               }
               first_time_adding = true;
               idx_ptr = _db_file.get_segment_manager()->construct< index_type >( type_name.c_str() )( index_alloc( _db_file.get_segment_manager() ) );
             }

            idx_ptr->validate();

            // Ensure the undo stack of added index is consistent with the other indices in the database
            if( _index_list.size() > 0 ) {
               auto expected_revision_range = _index_list.front()->undo_stack_revision_range();
               auto added_index_revision_range = idx_ptr->undo_stack_revision_range();

               if( added_index_revision_range.first != expected_revision_range.first ||
                   added_index_revision_range.second != expected_revision_range.second ) {

                  if( !first_time_adding ) {
                     BOOST_THROW_EXCEPTION( std::logic_error(
                        "existing index for " + type_name + " has an undo stack (revision range [" +
                        std::to_string(added_index_revision_range.first) + ", " + std::to_string(added_index_revision_range.second) +
                        "]) that is inconsistent with other indices in the database (revision range [" +
                        std::to_string(expected_revision_range.first) + ", " + std::to_string(expected_revision_range.second) +
                        "]); corrupted database?"
                     ) );
                  }

                  if( _read_only ) {
                     BOOST_THROW_EXCEPTION( std::logic_error(
                        "new index for " + type_name +
                        " requires an undo stack that is consistent with other indices in the database; cannot fix in read-only mode"
                     ) );
                  }

                  idx_ptr->set_revision( static_cast<uint64_t>(expected_revision_range.first) );
                  while( idx_ptr->revision() < expected_revision_range.second ) {
                     idx_ptr->start_undo_session(true).push();
                  }
               }
            }

            if( type_id >= _index_map.size() )
               _index_map.resize( type_id + 1 );

            auto new_index = new index<index_type>( *idx_ptr );
            _index_map[ type_id ].reset( new_index );
            _index_list.push_back( new_index );
         }

         auto get_segment_manager() -> decltype( ((pinnable_mapped_file*)nullptr)->get_segment_manager()) {
            return _db_file.get_segment_manager();
         }

         auto get_segment_manager()const -> std::add_const_t< decltype( ((pinnable_mapped_file*)nullptr)->get_segment_manager() ) > {
            return _db_file.get_segment_manager();
         }

         size_t get_free_memory()const
         {
            return _db_file.get_segment_manager()->get_free_memory();
         }

         template<typename MultiIndexType>
         const generic_index<MultiIndexType>& get_index()const
         {
            CHAINBASE_REQUIRE_READ_LOCK("get_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            assert( _index_map.size() > index_type::value_type::type_id );
            assert( _index_map[index_type::value_type::type_id] );
            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template<typename MultiIndexType, typename ByIndex>
         auto get_index()const -> decltype( ((generic_index<MultiIndexType>*)( nullptr ))->indices().template get<ByIndex>() )
         {
            CHAINBASE_REQUIRE_READ_LOCK("get_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            assert( _index_map.size() > index_type::value_type::type_id );
            assert( _index_map[index_type::value_type::type_id] );
            return index_type_ptr( _index_map[index_type::value_type::type_id]->get() )->indices().template get<ByIndex>();
         }

         template<typename MultiIndexType>
         generic_index<MultiIndexType>& get_mutable_index()
         {
            CHAINBASE_REQUIRE_WRITE_LOCK("get_mutable_index", typename MultiIndexType::value_type);
            typedef generic_index<MultiIndexType> index_type;
            typedef index_type*                   index_type_ptr;
            assert( _index_map.size() > index_type::value_type::type_id );
            assert( _index_map[index_type::value_type::type_id] );
            return *index_type_ptr( _index_map[index_type::value_type::type_id]->get() );
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType* find( CompatibleKey&& key )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("find", ObjectType);
             typedef typename get_index_type< ObjectType >::type index_type;
             const auto& idx = get_index< index_type >().indices().template get< IndexedByType >();
             auto itr = idx.find( std::forward< CompatibleKey >( key ) );
             if( itr == idx.end() ) return nullptr;
             return &*itr;
         }

         template< typename ObjectType >
         const ObjectType* find( oid< ObjectType > key = oid< ObjectType >() ) const
         {
             CHAINBASE_REQUIRE_READ_LOCK("find", ObjectType);
             typedef typename get_index_type< ObjectType >::type index_type;
             return get_index< index_type >().find( key );
         }

         template< typename ObjectType, typename IndexedByType, typename CompatibleKey >
         const ObjectType& get( CompatibleKey&& key )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("get", ObjectType);
             auto obj = find< ObjectType, IndexedByType >( std::forward< CompatibleKey >( key ) );
             if( !obj ) {
                std::stringstream ss;
                ss << "unknown key (" << boost::core::demangle( typeid( key ).name() ) << "): " << key;
                BOOST_THROW_EXCEPTION( std::out_of_range( ss.str().c_str() ) );
             }
             return *obj;
         }

         template< typename ObjectType >
         const ObjectType& get( const oid< ObjectType >& key = oid< ObjectType >() )const
         {
             CHAINBASE_REQUIRE_READ_LOCK("get", ObjectType);
             auto obj = find< ObjectType >( key );
             if( !obj ) {
                std::stringstream ss;
                ss << "unknown key (" << boost::core::demangle( typeid( key ).name() ) << "): " << key._id;
                BOOST_THROW_EXCEPTION( std::out_of_range( ss.str().c_str() ) );
             }
             return *obj;
         }

         template<typename ObjectType, typename Modifier>
         void modify( const ObjectType& obj, Modifier&& m )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("modify", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             get_mutable_index<index_type>().modify( obj, m );
         }

         template<typename ObjectType>
         void remove( const ObjectType& obj )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("remove", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             return get_mutable_index<index_type>().remove( obj );
         }

         template<typename ObjectType, typename Constructor>
         const ObjectType& create( Constructor&& con )
         {
             CHAINBASE_REQUIRE_WRITE_LOCK("create", ObjectType);
             typedef typename get_index_type<ObjectType>::type index_type;
             return get_mutable_index<index_type>().emplace( std::forward<Constructor>(con) );
         }

         database_index_row_count_multiset row_count_per_index()const {
            database_index_row_count_multiset ret;
            for(const auto& ai_ptr : _index_map) {
               if(!ai_ptr)
                  continue;
               ret.emplace(make_pair(ai_ptr->row_count(), ai_ptr->type_name()));
            }
            return ret;
         }

      private:
         pinnable_mapped_file                                        _db_file;
         bool                                                        _read_only = false;

         /**
          * This is a sparse list of known indices kept to accelerate creation of undo sessions
          */
         vector<abstract_index*>                                     _index_list;

         /**
          * This is a full map (size 2^16) of all possible index designed for constant time lookup
          */
         vector<unique_ptr<abstract_index>>                          _index_map;

#ifdef CHAINBASE_CHECK_LOCKING
         int32_t                                                     _read_lock_count = 0;
         int32_t                                                     _write_lock_count = 0;
         bool                                                        _enable_require_locking = false;
#endif
   };

   template<typename Object, typename... Args>
   using shared_multi_index_container = boost::multi_index_container<Object,Args..., chainbase::node_allocator<Object> >;
}  // namepsace chainbase
