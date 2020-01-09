#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

namespace chainbase { class database; }

namespace eosio { namespace chain {

class controller;
class transaction_context;

class apply_context {
   private:
      template<typename T>
      class iterator_cache {
         public:
            iterator_cache(){
               _end_iterator_to_table.reserve(8);
               _iterator_to_object.reserve(32);
            }

            /// Returns end iterator of the table.
            int cache_table( const table_id_object& tobj ) {
               auto itr = _table_cache.find(tobj.id);
               if( itr != _table_cache.end() )
                  return itr->second.second;

               auto ei = index_to_end_iterator(_end_iterator_to_table.size());
               _end_iterator_to_table.push_back( &tobj );
               _table_cache.emplace( tobj.id, make_pair(&tobj, ei) );
               return ei;
            }

            const table_id_object& get_table( table_id_object::id_type i )const {
               auto itr = _table_cache.find(i);
               EOS_ASSERT( itr != _table_cache.end(), table_not_in_cache, "an invariant was broken, table should be in cache" );
               return *itr->second.first;
            }

            int get_end_iterator_by_table_id( table_id_object::id_type i )const {
               auto itr = _table_cache.find(i);
               EOS_ASSERT( itr != _table_cache.end(), table_not_in_cache, "an invariant was broken, table should be in cache" );
               return itr->second.second;
            }

            const table_id_object* find_table_by_end_iterator( int ei )const {
               EOS_ASSERT( ei < -1, invalid_table_iterator, "not an end iterator" );
               auto indx = end_iterator_to_index(ei);
               if( indx >= _end_iterator_to_table.size() ) return nullptr;
               return _end_iterator_to_table[indx];
            }

            const T& get( int iterator ) {
               EOS_ASSERT( iterator != -1, invalid_table_iterator, "invalid iterator" );
               EOS_ASSERT( iterator >= 0, table_operation_not_permitted, "dereference of end iterator" );
               EOS_ASSERT( (size_t)iterator < _iterator_to_object.size(), invalid_table_iterator, "iterator out of range" );
               auto result = _iterator_to_object[iterator];
               EOS_ASSERT( result, table_operation_not_permitted, "dereference of deleted object" );
               return *result;
            }

            void remove( int iterator ) {
               EOS_ASSERT( iterator != -1, invalid_table_iterator, "invalid iterator" );
               EOS_ASSERT( iterator >= 0, table_operation_not_permitted, "cannot call remove on end iterators" );
               EOS_ASSERT( (size_t)iterator < _iterator_to_object.size(), invalid_table_iterator, "iterator out of range" );

               auto obj_ptr = _iterator_to_object[iterator];
               if( !obj_ptr ) return;
               _iterator_to_object[iterator] = nullptr;
               _object_to_iterator.erase( obj_ptr );
            }

            int add( const T& obj ) {
               auto itr = _object_to_iterator.find( &obj );
               if( itr != _object_to_iterator.end() )
                    return itr->second;

               _iterator_to_object.push_back( &obj );
               _object_to_iterator[&obj] = _iterator_to_object.size() - 1;

               return _iterator_to_object.size() - 1;
            }

         private:
            map<table_id_object::id_type, pair<const table_id_object*, int>> _table_cache;
            vector<const table_id_object*>                  _end_iterator_to_table;
            vector<const T*>                                _iterator_to_object;
            map<const T*,int>                               _object_to_iterator;

            /// Precondition: std::numeric_limits<int>::min() < ei < -1
            /// Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
            inline size_t end_iterator_to_index( int ei )const { return (-ei - 2); }
            /// Precondition: indx < _end_iterator_to_table.size() <= std::numeric_limits<int>::max()
            inline int index_to_end_iterator( size_t indx )const { return -(indx + 2); }
      }; /// class iterator_cache

      template<typename>
      struct array_size;

      template<typename T, size_t N>
      struct array_size< std::array<T,N> > {
          static constexpr size_t size = N;
      };

      template <typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst, typename Enable = void>
      class secondary_key_helper;

      template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
      class secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
         typename std::enable_if<std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value>::type >
      {
         public:
            typedef SecondaryKey secondary_key_type;

            static void set(secondary_key_type& sk_in_table, const secondary_key_type& sk_from_wasm) {
               sk_in_table = sk_from_wasm;
            }

            static void get(secondary_key_type& sk_from_wasm, const secondary_key_type& sk_in_table ) {
               sk_from_wasm = sk_in_table;
            }

            static auto create_tuple(const table_id_object& tab, const secondary_key_type& secondary) {
               return boost::make_tuple( tab.id, secondary );
            }
      };

      template<typename SecondaryKey, typename SecondaryKeyProxy, typename SecondaryKeyProxyConst>
      class secondary_key_helper<SecondaryKey, SecondaryKeyProxy, SecondaryKeyProxyConst,
         typename std::enable_if<!std::is_same<SecondaryKey, typename std::decay<SecondaryKeyProxy>::type>::value &&
                                 std::is_pointer<typename std::decay<SecondaryKeyProxy>::type>::value>::type >
      {
         public:
            typedef SecondaryKey      secondary_key_type;
            typedef SecondaryKeyProxy secondary_key_proxy_type;
            typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

            static constexpr size_t N = array_size<SecondaryKey>::size;

            static void set(secondary_key_type& sk_in_table, secondary_key_proxy_const_type sk_from_wasm) {
               std::copy(sk_from_wasm, sk_from_wasm + N, sk_in_table.begin());
            }

            static void get(secondary_key_proxy_type sk_from_wasm, const secondary_key_type& sk_in_table) {
               std::copy(sk_in_table.begin(), sk_in_table.end(), sk_from_wasm);
            }

            static auto create_tuple(const table_id_object& tab, secondary_key_proxy_const_type sk_from_wasm) {
               secondary_key_type secondary;
               std::copy(sk_from_wasm, sk_from_wasm + N, secondary.begin());
               return boost::make_tuple( tab.id, secondary );
            }
      };

   public:

      struct read_only_context {
         explicit constexpr read_only_context(const chainbase::database& db) : db{const_cast<chainbase::database&>(db)} {}
         const table_id_object* find_table( name code, name scope, name table );
         chainbase::database& db;
      };

      template<typename ObjectType,
               typename SecondaryKeyProxy = typename std::add_lvalue_reference<typename ObjectType::secondary_key_type>::type,
               typename SecondaryKeyProxyConst = typename std::add_lvalue_reference<
                                                   typename std::add_const<typename ObjectType::secondary_key_type>::type>::type >
      class generic_index_read_only
      {
         public:
            typedef typename ObjectType::secondary_key_type secondary_key_type;
            typedef SecondaryKeyProxy      secondary_key_proxy_type;
            typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

            using secondary_key_helper_t = secondary_key_helper<secondary_key_type, secondary_key_proxy_type, secondary_key_proxy_const_type>;

            generic_index_read_only( const chainbase::database& db ): context{db} {}

            int find_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_const_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto* obj = context.db.find<ObjectType, by_secondary>( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( !obj ) return table_end_itr;

               primary = obj->primary_key;

               return itr_cache.add( *obj );
            }

            int lowerbound_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index< typename chainbase::get_index_type<ObjectType>::type, by_secondary >();
               auto itr = idx.lower_bound( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( itr == idx.end() ) return table_end_itr;
               if( itr->t_id != tab->id ) return table_end_itr;

               primary = itr->primary_key;
               secondary_key_helper_t::get(secondary, itr->secondary_key);

               return itr_cache.add( *itr );
            }

            int upperbound_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index< typename chainbase::get_index_type<ObjectType>::type, by_secondary >();
               auto itr = idx.upper_bound( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( itr == idx.end() ) return table_end_itr;
               if( itr->t_id != tab->id ) return table_end_itr;

               primary = itr->primary_key;
               secondary_key_helper_t::get(secondary, itr->secondary_key);

               return itr_cache.add( *itr );
            }

            int end_secondary( uint64_t code, uint64_t scope, uint64_t table ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if( !tab ) return -1;

               return itr_cache.cache_table( *tab );
            }

            int next_secondary( int iterator, uint64_t& primary ) {
               if( iterator < -1 ) return -1; // cannot increment past end iterator of index

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_secondary>();

               auto itr = idx.iterator_to(obj);
               ++itr;

               if( itr == idx.end() || itr->t_id != obj.t_id ) return itr_cache.get_end_iterator_by_table_id(obj.t_id);

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            int previous_secondary( int iterator, uint64_t& primary ) {
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_secondary>();

               if( iterator < -1 ) // is end iterator
               {
                  auto tab = itr_cache.find_table_by_end_iterator(iterator);
                  EOS_ASSERT( tab, invalid_table_iterator, "not a valid end iterator" );

                  auto itr = idx.upper_bound(tab->id);
                  if( idx.begin() == idx.end() || itr == idx.begin() ) return -1; // Empty index

                  --itr;

                  if( itr->t_id != tab->id ) return -1; // Empty index

                  primary = itr->primary_key;
                  return itr_cache.add(*itr);
               }

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call

               auto itr = idx.iterator_to(obj);
               if( itr == idx.begin() ) return -1; // cannot decrement past beginning iterator of index

               --itr;

               if( itr->t_id != obj.t_id ) return -1; // cannot decrement past beginning iterator of index

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            int find_primary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_type secondary, uint64_t primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto* obj = context.db.find<ObjectType, by_primary>( boost::make_tuple( tab->id, primary ) );
               if( !obj ) return table_end_itr;
               secondary_key_helper_t::get(secondary, obj->secondary_key);

               return itr_cache.add( *obj );
            }

            int lowerbound_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if (!tab) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_primary>();
               auto itr = idx.lower_bound(boost::make_tuple(tab->id, primary));
               if (itr == idx.end()) return table_end_itr;
               if (itr->t_id != tab->id) return table_end_itr;

               return itr_cache.add(*itr);
            }

            int upperbound_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary ) {
               auto tab = context.find_table( name(code), name(scope), name(table) );
               if ( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_primary>();
               auto itr = idx.upper_bound(boost::make_tuple(tab->id, primary));
               if (itr == idx.end()) return table_end_itr;
               if (itr->t_id != tab->id) return table_end_itr;

               itr_cache.cache_table(*tab);
               return itr_cache.add(*itr);
            }

            int next_primary( int iterator, uint64_t& primary ) {
               if( iterator < -1 ) return -1; // cannot increment past end iterator of table

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_primary>();

               auto itr = idx.iterator_to(obj);
               ++itr;

               if( itr == idx.end() || itr->t_id != obj.t_id ) return itr_cache.get_end_iterator_by_table_id(obj.t_id);

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            int previous_primary( int iterator, uint64_t& primary ) {
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, by_primary>();

               if( iterator < -1 ) // is end iterator
               {
                  auto tab = itr_cache.find_table_by_end_iterator(iterator);
                  EOS_ASSERT( tab, invalid_table_iterator, "not a valid end iterator" );

                  auto itr = idx.upper_bound(tab->id);
                  if( idx.begin() == idx.end() || itr == idx.begin() ) return -1; // Empty table

                  --itr;

                  if( itr->t_id != tab->id ) return -1; // Empty table

                  primary = itr->primary_key;
                  return itr_cache.add(*itr);
               }

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call

               auto itr = idx.iterator_to(obj);
               if( itr == idx.begin() ) return -1; // cannot decrement past beginning iterator of table

               --itr;

               if( itr->t_id != obj.t_id ) return -1; // cannot decrement past beginning iterator of index

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            void get( int iterator, uint64_t& primary, secondary_key_proxy_type secondary ) {
               const auto& obj = itr_cache.get( iterator );
               primary   = obj.primary_key;
               secondary_key_helper_t::get(secondary, obj.secondary_key);
            }

         protected:
            read_only_context           context;
            iterator_cache<ObjectType>  itr_cache;
      }; /// class generic_index_read_only

      template<typename ObjectType,
               typename SecondaryKeyProxy = typename std::add_lvalue_reference<typename ObjectType::secondary_key_type>::type,
               typename SecondaryKeyProxyConst = typename std::add_lvalue_reference<
                                                   typename std::add_const<typename ObjectType::secondary_key_type>::type>::type >
      class generic_index : public generic_index_read_only<ObjectType, SecondaryKeyProxy, SecondaryKeyProxyConst>
      {
         public:
            typedef typename ObjectType::secondary_key_type secondary_key_type;
            typedef SecondaryKeyProxy      secondary_key_proxy_type;
            typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

            using secondary_key_helper_t = secondary_key_helper<secondary_key_type, secondary_key_proxy_type, secondary_key_proxy_const_type>;

            generic_index( apply_context& c ): generic_index_read_only<ObjectType, SecondaryKeyProxy, SecondaryKeyProxyConst>(c.db), context(c){}

            int store( uint64_t scope, uint64_t table, const account_name& payer,
                       uint64_t id, secondary_key_proxy_const_type value )
            {
               EOS_ASSERT( payer != account_name(), invalid_table_payer, "must specify a valid account to pay for new record" );

//               context.require_write_lock( scope );

               const auto& tab = context.find_or_create_table( context.receiver, name(scope), name(table), payer );

               const auto& obj = context.db.create<ObjectType>( [&]( auto& o ){
                  o.t_id          = tab.id;
                  o.primary_key   = id;
                  secondary_key_helper_t::set(o.secondary_key, value);
                  o.payer         = payer;
               });

               context.db.modify( tab, [&]( auto& t ) {
                 ++t.count;
               });

               context.update_db_usage( payer, config::billable_size_v<ObjectType> );

               itr_cache.cache_table( tab );
               return itr_cache.add( obj );
            }

            void remove( int iterator ) {
               const auto& obj = itr_cache.get( iterator );
               context.update_db_usage( obj.payer, -( config::billable_size_v<ObjectType> ) );

               const auto& table_obj = itr_cache.get_table( obj.t_id );
               EOS_ASSERT( table_obj.code == context.receiver, table_access_violation, "db access violation" );

//               context.require_write_lock( table_obj.scope );

               context.db.modify( table_obj, [&]( auto& t ) {
                  --t.count;
               });
               context.db.remove( obj );

               if (table_obj.count == 0) {
                  context.remove_table(table_obj);
               }

               itr_cache.remove( iterator );
            }

            void update( int iterator, account_name payer, secondary_key_proxy_const_type secondary ) {
               const auto& obj = itr_cache.get( iterator );

               const auto& table_obj = itr_cache.get_table( obj.t_id );
               EOS_ASSERT( table_obj.code == context.receiver, table_access_violation, "db access violation" );

//               context.require_write_lock( table_obj.scope );

               if( payer == account_name() ) payer = obj.payer;

               int64_t billing_size =  config::billable_size_v<ObjectType>;

               if( obj.payer != payer ) {
                  context.update_db_usage( obj.payer, -(billing_size) );
                  context.update_db_usage( payer, +(billing_size) );
               }

               context.db.modify( obj, [&]( auto& o ) {
                 secondary_key_helper_t::set(o.secondary_key, secondary);
                 o.payer = payer;
               });
            }
         private:
            using generic_index_read_only<ObjectType, SecondaryKeyProxy, SecondaryKeyProxyConst>::itr_cache;
            apply_context& context;
      }; /// class generic_index

   /// Constructor
   public:
      apply_context(controller& con, transaction_context& trx_ctx, uint32_t action_ordinal, uint32_t depth=0);

   /// Execution methods:
   public:

      void exec_one();
      void exec();
      void execute_inline( action&& a );
      void execute_context_free_inline( action&& a );
      void schedule_deferred_transaction( const uint128_t& sender_id, account_name payer, transaction&& trx, bool replace_existing );
      bool cancel_deferred_transaction( const uint128_t& sender_id, account_name sender );
      bool cancel_deferred_transaction( const uint128_t& sender_id ) { return cancel_deferred_transaction(sender_id, receiver); }

   protected:
      uint32_t schedule_action( uint32_t ordinal_of_action_to_schedule, account_name receiver, bool context_free );
      uint32_t schedule_action( action&& act_to_schedule, account_name receiver, bool context_free );


   /// Authorization methods:
   public:

      /**
       * @brief Require @ref account to have approved of this message
       * @param account The account whose approval is required
       *
       * This method will check that @ref account is listed in the message's declared authorizations, and marks the
       * authorization as used. Note that all authorizations on a message must be used, or the message is invalid.
       *
       * @throws missing_auth_exception If no sufficient permission was found
       */
      void require_authorization(const account_name& account);
      bool has_authorization(const account_name& account) const;
      void require_authorization(const account_name& account, const permission_name& permission);

      /**
       * @return true if account exists, false if it does not
       */
      bool is_account(const account_name& account)const;

      /**
       * Requires that the current action be delivered to account
       */
      void require_recipient(account_name account);

      /**
       * Return true if the current action has already been scheduled to be
       * delivered to the specified account.
       */
      bool has_recipient(account_name account)const;

   /// Console methods:
   public:

      void console_append( const string& val ) {
         _pending_console_output += val;
      }

   /// Database methods:
   public:

      class primary_index_read_only : public read_only_context {
       public:
         primary_index_read_only( const chainbase::database& db ) : read_only_context{db} {}
         int  db_get_i64( int iterator, char* buffer, size_t buffer_size );
         int  db_next_i64( int iterator, uint64_t& primary );
         int  db_previous_i64( int iterator, uint64_t& primary );
         int  db_find_i64( name code, name scope, name table, uint64_t id );
         int  db_lowerbound_i64( name code, name scope, name table, uint64_t id );
         int  db_upperbound_i64( name code, name scope, name table, uint64_t id );
         int  db_end_i64( name code, name scope, name table );
         iterator_cache<key_value_object>    keyval_cache;
      };

      void update_db_usage( const account_name& payer, int64_t delta );

      int  db_store_i64( name scope, name table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size );
      void db_update_i64( int iterator, account_name payer, const char* buffer, size_t buffer_size );
      void db_remove_i64( int iterator );
      int  db_get_i64( int iterator, char* buffer, size_t buffer_size );
      int  db_next_i64( int iterator, uint64_t& primary );
      int  db_previous_i64( int iterator, uint64_t& primary );
      int  db_find_i64( name code, name scope, name table, uint64_t id );
      int  db_lowerbound_i64( name code, name scope, name table, uint64_t id );
      int  db_upperbound_i64( name code, name scope, name table, uint64_t id );
      int  db_end_i64( name code, name scope, name table );

   private:

      const table_id_object* find_table( name code, name scope, name table );
      const table_id_object& find_or_create_table( name code, name scope, name table, const account_name &payer );
      void                   remove_table( const table_id_object& tid );

      int  db_store_i64( name code, name scope, name table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size );


   /// Misc methods:
   public:


      int get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const;
      int get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const;
      vector<account_name> get_active_producers() const;
      bytes  get_packed_transaction();

      uint64_t next_global_sequence();
      uint64_t next_recv_sequence( const account_metadata_object& receiver_account );
      uint64_t next_auth_sequence( account_name actor );

      void add_ram_usage( account_name account, int64_t ram_delta );
      void finalize_trace( action_trace& trace, const fc::time_point& start );

      bool is_context_free()const { return context_free; }
      bool is_privileged()const { return privileged; }
      action_name get_receiver()const { return receiver; }
      const action& get_action()const { return *act; }

      action_name get_sender() const;

   /// Fields:
   public:

      controller&                   control;
      chainbase::database&          db;  ///< database where state is stored
      transaction_context&          trx_context; ///< transaction context in which the action is running

   private:
      const action*                 act = nullptr; ///< action being applied
      // act pointer may be invalidated on call to trx_context.schedule_action
      account_name                  receiver; ///< the code that is currently running
      uint32_t                      recurse_depth; ///< how deep inline actions can recurse
      uint32_t                      first_receiver_action_ordinal = 0;
      uint32_t                      action_ordinal = 0;
      bool                          privileged   = false;
      bool                          context_free = false;

   public:
      std::vector<char>             action_return_value;
      generic_index<index64_object>                                  idx64;
      generic_index<index128_object>                                 idx128;
      generic_index<index256_object, uint128_t*, const uint128_t*>   idx256;
      generic_index<index_double_object>                             idx_double;
      generic_index<index_long_double_object>                        idx_long_double;
      primary_index_read_only                                        idx_primary;

   private:

      vector< std::pair<account_name, uint32_t> > _notified; ///< keeps track of new accounts to be notifed of current message
      vector<uint32_t>                    _inline_actions; ///< action_ordinals of queued inline actions
      vector<uint32_t>                    _cfa_inline_actions; ///< action_ordinals of queued inline context-free actions
      std::string                         _pending_console_output;
      flat_set<account_delta>             _account_ram_deltas; ///< flat_set of account_delta so json is an array of objects

      //bytes                               _cached_trx;
};

using apply_handler = std::function<void(apply_context&)>;

} } // namespace eosio::chain

//FC_REFLECT(eosio::chain::apply_context::apply_results, (applied_actions)(deferred_transaction_requests)(deferred_transactions_count))
