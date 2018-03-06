/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/block.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>

namespace chainbase { class database; }

namespace eosio { namespace chain {

using contracts::key_value_object;

class chain_controller;

class apply_context {
   private:
      template<typename T>
      class iterator_cache {
         public:
            typedef contracts::table_id_object table_id_object;

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
               FC_ASSERT( itr != _table_cache.end(), "an invariant was broken, table should be in cache" );
               return *itr->second.first;
            }

            int get_end_iterator_by_table_id( table_id_object::id_type i )const {
               auto itr = _table_cache.find(i);
               FC_ASSERT( itr != _table_cache.end(), "an invariant was broken, table should be in cache" );
               return itr->second.second;
            }

            const table_id_object* find_table_by_end_iterator( int ei )const {
               FC_ASSERT( ei < -1, "not an end iterator" );
               auto indx = end_iterator_to_index(ei);
               if( indx >= _end_iterator_to_table.size() ) return nullptr;
               return _end_iterator_to_table[indx];
            }

            const T& get( int iterator ) {
               FC_ASSERT( iterator != -1, "invalid iterator" );
               FC_ASSERT( iterator >= 0, "dereference of end iterator" );
               FC_ASSERT( iterator < _iterator_to_object.size(), "iterator out of range" );
               auto result = _iterator_to_object[iterator];
               FC_ASSERT( result, "dereference of deleted object" );
               return *result;
            }

            void remove( int iterator ) {
               FC_ASSERT( iterator != -1, "invalid iterator" );
               FC_ASSERT( iterator >= 0, "cannot call remove on end iterators" );
               FC_ASSERT( iterator < _iterator_to_object.size(), "iterator out of range" );
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
      };

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

            static auto create_tuple(const contracts::table_id_object& tab, const secondary_key_type& secondary) {
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

            static auto create_tuple(const contracts::table_id_object& tab, secondary_key_proxy_const_type sk_from_wasm) {
               secondary_key_type secondary;
               std::copy(sk_from_wasm, sk_from_wasm + N, secondary.begin());
               return boost::make_tuple( tab.id, secondary );
            }
      };

   public:
      template<typename ObjectType,
               typename SecondaryKeyProxy = typename std::add_lvalue_reference<typename ObjectType::secondary_key_type>::type,
               typename SecondaryKeyProxyConst = typename std::add_lvalue_reference<
                                                   typename std::add_const<typename ObjectType::secondary_key_type>::type>::type >
      class generic_index
      {
         public:
            typedef typename ObjectType::secondary_key_type secondary_key_type;
            typedef SecondaryKeyProxy      secondary_key_proxy_type;
            typedef SecondaryKeyProxyConst secondary_key_proxy_const_type;

            using secondary_key_helper_t = secondary_key_helper<secondary_key_type, secondary_key_proxy_type, secondary_key_proxy_const_type>;

            generic_index( apply_context& c ):context(c){}

            int store( uint64_t scope, uint64_t table, const account_name& payer,
                       uint64_t id, secondary_key_proxy_const_type value )
            {
               FC_ASSERT( payer != account_name(), "must specify a valid account to pay for new record" );

               context.require_write_lock( scope );

               const auto& tab = context.find_or_create_table( context.receiver, scope, table );

               const auto& obj = context.mutable_db.create<ObjectType>( [&]( auto& o ){
                  o.t_id          = tab.id;
                  o.primary_key   = id;
                  secondary_key_helper_t::set(o.secondary_key, value);
                  o.payer         = payer;
               });

               context.mutable_db.modify( tab, [&]( auto& t ) {
                 ++t.count;
               });

               context.update_db_usage( payer, contracts::get_key_memory_usage<secondary_key_type>()+base_row_fee );

               itr_cache.cache_table( tab );
               return itr_cache.add( obj );
            }

            void remove( int iterator ) {
               const auto& obj = itr_cache.get( iterator );
               context.update_db_usage( obj.payer, -( contracts::get_key_memory_usage<secondary_key_type>()+base_row_fee ) );

               const auto& table_obj = itr_cache.get_table( obj.t_id );
               context.require_write_lock( table_obj.scope );

               context.mutable_db.modify( table_obj, [&]( auto& t ) {
                  --t.count;
               });
               context.mutable_db.remove( obj );

               itr_cache.remove( iterator );
            }

            void update( int iterator, account_name payer, secondary_key_proxy_const_type secondary ) {
               const auto& obj = itr_cache.get( iterator );

               context.require_write_lock( itr_cache.get_table( obj.t_id ).scope );

               if( payer == account_name() ) payer = obj.payer;

               if( obj.payer != payer ) {
                  context.update_db_usage( obj.payer, -(contracts::get_key_memory_usage<secondary_key_type>()+base_row_fee) );
                  context.update_db_usage( payer, +(contracts::get_key_memory_usage<secondary_key_type>()+base_row_fee) );
               }

               context.mutable_db.modify( obj, [&]( auto& o ) {
                 secondary_key_helper_t::set(o.secondary_key, secondary);
                 o.payer = payer;
               });
            }

            int find_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_const_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( code, scope, table );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto* obj = context.db.find<ObjectType, contracts::by_secondary>( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( !obj ) return table_end_itr;

               primary = obj->primary_key;

               return itr_cache.add( *obj );
            }

            int lowerbound_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( code, scope, table );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index< typename chainbase::get_index_type<ObjectType>::type, contracts::by_secondary >();
               auto itr = idx.lower_bound( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( itr == idx.end() ) return table_end_itr;
               if( itr->t_id != tab->id ) return table_end_itr;

               primary = itr->primary_key;
               secondary_key_helper_t::get(secondary, itr->secondary_key);

               return itr_cache.add( *itr );
            }

            int upperbound_secondary( uint64_t code, uint64_t scope, uint64_t table, secondary_key_proxy_type secondary, uint64_t& primary ) {
               auto tab = context.find_table( code, scope, table );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index< typename chainbase::get_index_type<ObjectType>::type, contracts::by_secondary >();
               auto itr = idx.upper_bound( secondary_key_helper_t::create_tuple( *tab, secondary ) );
               if( itr == idx.end() ) return table_end_itr;
               if( itr->t_id != tab->id ) return table_end_itr;

               primary = itr->primary_key;
               secondary_key_helper_t::get(secondary, itr->secondary_key);

               return itr_cache.add( *itr );
            }

            int end_secondary( uint64_t code, uint64_t scope, uint64_t table ) {
               auto tab = context.find_table( code, scope, table );
               if( !tab ) return -1;

               return itr_cache.cache_table( *tab );
            }

            int next_secondary( int iterator, uint64_t& primary ) {
               if( iterator < -1 ) return -1; // cannot increment past end iterator of index

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_secondary>();

               auto itr = idx.iterator_to(obj);
               ++itr;

               if( itr == idx.end() || itr->t_id != obj.t_id ) return itr_cache.get_end_iterator_by_table_id(obj.t_id);

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            int previous_secondary( int iterator, uint64_t& primary ) {
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_secondary>();

               if( iterator < -1 ) // is end iterator
               {
                  auto tab = itr_cache.find_table_by_end_iterator(iterator);
                  FC_ASSERT( tab, "not a valid end iterator" );

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
               auto tab = context.find_table( code, scope, table );
               if( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto* obj = context.db.find<ObjectType, contracts::by_primary>( boost::make_tuple( tab->id, primary ) );
               if( !obj ) return table_end_itr;
               secondary_key_helper_t::get(secondary, obj->secondary_key);

               return itr_cache.add( *obj );
            }

            int lowerbound_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary ) {
               auto tab = context.find_table( code, scope, table );
               if (!tab) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_primary>();
               auto itr = idx.lower_bound(boost::make_tuple(tab->id, primary));
               if (itr == idx.end()) return table_end_itr;
               if (itr->t_id != tab->id) return table_end_itr;

               return itr_cache.add(*itr);
            }

            int upperbound_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary ) {
               auto tab = context.find_table( code, scope, table );
               if ( !tab ) return -1;

               auto table_end_itr = itr_cache.cache_table( *tab );

               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_primary>();
               auto itr = idx.upper_bound(boost::make_tuple(tab->id, primary));
               if (itr == idx.end()) return table_end_itr;
               if (itr->t_id != tab->id) return table_end_itr;

               itr_cache.cache_table(*tab);
               return itr_cache.add(*itr);
            }

            int next_primary( int iterator, uint64_t& primary ) {
               if( iterator < -1 ) return -1; // cannot increment past end iterator of table

               const auto& obj = itr_cache.get(iterator); // Check for iterator != -1 happens in this call
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_primary>();

               auto itr = idx.iterator_to(obj);
               ++itr;

               if( itr == idx.end() || itr->t_id != obj.t_id ) return itr_cache.get_end_iterator_by_table_id(obj.t_id);

               primary = itr->primary_key;
               return itr_cache.add(*itr);
            }

            int previous_primary( int iterator, uint64_t& primary ) {
               const auto& idx = context.db.get_index<typename chainbase::get_index_type<ObjectType>::type, contracts::by_primary>();

               if( iterator < -1 ) // is end iterator
               {
                  auto tab = itr_cache.find_table_by_end_iterator(iterator);
                  FC_ASSERT( tab, "not a valid end iterator" );

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

         private:
            apply_context&              context;
            iterator_cache<ObjectType>  itr_cache;
      };




      apply_context(chain_controller& con, chainbase::database& db, const action& a, const transaction_metadata& trx_meta, uint32_t depth=0)

      :controller(con),
       db(db),
       act(a),
       mutable_controller(con),
       mutable_db(db),
       used_authorizations(act.authorization.size(), false),
       trx_meta(trx_meta),
       idx64(*this),
       idx128(*this),
       idx256(*this),
       recurse_depth(depth)
       {}

      void exec();

      void execute_inline( action &&a );
      void execute_deferred( deferred_transaction &&trx );
      void cancel_deferred( uint32_t sender_id );

      using table_id_object = contracts::table_id_object;
      const table_id_object* find_table( name code, name scope, name table );
      const table_id_object& find_or_create_table( name code, name scope, name table );

      template <typename ObjectType>
      int32_t store_record( const table_id_object& t_id, const account_name& bta, const typename ObjectType::key_type* keys, const char* value, size_t valuelen );

      template <typename ObjectType>
      int32_t update_record( const table_id_object& t_id, const account_name& bta, const typename ObjectType::key_type* keys, const char* value, size_t valuelen );

      template <typename ObjectType>
      int32_t remove_record( const table_id_object& t_id, const typename ObjectType::key_type* keys );

      template <typename IndexType, typename Scope>
      int32_t load_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );


      template <typename IndexType, typename Scope>
      int32_t front_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );

      template <typename IndexType, typename Scope>
      int32_t back_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys,
                           char* value, size_t valuelen );

      template <typename IndexType, typename Scope>
      int32_t next_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );

      template <typename IndexType, typename Scope>
      int32_t previous_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );

      template <typename IndexType, typename Scope>
      int32_t lower_bound_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );

      template <typename IndexType, typename Scope>
      int32_t upper_bound_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen );

      /**
       * @brief Require @ref account to have approved of this message
       * @param account The account whose approval is required
       *
       * This method will check that @ref account is listed in the message's declared authorizations, and marks the
       * authorization as used. Note that all authorizations on a message must be used, or the message is invalid.
       *
       * @throws tx_missing_auth If no sufficient permission was found
       */
      void require_authorization(const account_name& account)const;
      void require_authorization(const account_name& account, const permission_name& permission)const;
      void require_write_lock(const scope_name& scope);
      void require_read_lock(const account_name& account, const scope_name& scope);

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

      bool                     all_authorizations_used()const;
      vector<permission_level> unused_authorizations()const;

      vector<account_name> get_active_producers() const;

      const bytes&         get_packed_transaction();

      const chain_controller&       controller;
      const chainbase::database&    db;  ///< database where state is stored
      const action&                 act; ///< message being applied
      account_name                  receiver; ///< the code that is currently running
      bool                          privileged   = false;
      bool                          context_free = false;
      bool                          used_context_free_api = false;

      chain_controller&             mutable_controller;
      chainbase::database&          mutable_db;


      ///< Parallel to act.authorization; tracks which permissions have been used while processing the message
      vector<bool> used_authorizations;

      const transaction_metadata&   trx_meta;

      struct apply_results {
         vector<action_trace>          applied_actions;
         vector<deferred_transaction>  generated_transactions;
         vector<deferred_reference>    canceled_deferred;
      };

      apply_results results;

      template<typename T>
      void console_append(T val) {
         _pending_console_output << val;
      }

      template<typename T, typename ...Ts>
      void console_append(T val, Ts ...rest) {
         console_append(val);
         console_append(rest...);
      };

      inline void console_append_formatted(const string& fmt, const variant_object& vo) {
         console_append(fc::format_string(fmt, vo));
      }

      void checktime(uint32_t instruction_count) const;

      int get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const;
      int get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const;

      void update_db_usage( const account_name& payer, int64_t delta );
      int  db_store_i64( uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size );
      void db_update_i64( int iterator, account_name payer, const char* buffer, size_t buffer_size );
      void db_remove_i64( int iterator );
      int db_get_i64( int iterator, char* buffer, size_t buffer_size );
      int db_next_i64( int iterator, uint64_t& primary );
      int db_previous_i64( int iterator, uint64_t& primary );
      int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
      int db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
      int db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
      int db_end_i64( uint64_t code, uint64_t scope, uint64_t table );

      generic_index<contracts::index64_object>    idx64;
      generic_index<contracts::index128_object>   idx128;
      generic_index<contracts::index256_object, uint128_t*, const uint128_t*>   idx256;

      uint32_t                                    recurse_depth;  // how deep inline actions can recurse

      static constexpr uint32_t base_row_fee = 200;

   private:
      iterator_cache<key_value_object> keyval_cache;

      void append_results(apply_results &&other) {
         fc::move_append(results.applied_actions, move(other.applied_actions));
         fc::move_append(results.generated_transactions, move(other.generated_transactions));
         fc::move_append(results.canceled_deferred, move(other.canceled_deferred));
      }

      void exec_one();

      void validate_table_key( const table_id_object& t_id, contracts::table_key_type key_type );

      void validate_or_add_table_key( const table_id_object& t_id, contracts::table_key_type key_type );

      template<typename ObjectType>
      static contracts::table_key_type get_key_type();

      vector<account_name>                _notified; ///< keeps track of new accounts to be notifed of current message
      vector<action>                      _inline_actions; ///< queued inline messages
      std::ostringstream                  _pending_console_output;

      vector<shard_lock>                  _read_locks;
      vector<scope_name>                  _write_scopes;
      bytes                               _cached_trx;
};

using apply_handler = std::function<void(apply_context&)>;

   namespace impl {
      template<typename Scope>
      struct scope_to_key_index;

      template<>
      struct scope_to_key_index<contracts::by_scope_primary> {
         static constexpr int value = 0;
      };

      template<>
      struct scope_to_key_index<contracts::by_scope_secondary> {
         static constexpr int value = 1;
      };

      template<>
      struct scope_to_key_index<contracts::by_scope_tertiary> {
         static constexpr int value = 2;
      };

      template<typename Scope>
      constexpr int scope_to_key_index_v = scope_to_key_index<Scope>::value;

      template<int>
      struct object_key_value;

      template<>
      struct object_key_value<0> {
         template<typename ObjectType>
         static const auto& get(const ObjectType& o) {
            return o.primary_key;
         }

         template<typename ObjectType>
         static auto& get(ObjectType& o) {
            return o.primary_key;
         }
      };

      template<>
      struct object_key_value<1> {
         template<typename ObjectType>
         static const auto& get(const ObjectType& o) {
            return o.secondary_key;
         }

         template<typename ObjectType>
         static auto& get(ObjectType& o) {
            return o.secondary_key;
         }
      };

      template<>
      struct object_key_value<2> {
         template<typename ObjectType>
         static const auto& get(const ObjectType& o) {
            return o.tertiary_key;
         }

         template<typename ObjectType>
         static auto& get( ObjectType& o) {
            return o.tertiary_key;
         }
      };

      template<typename KeyType>
      const KeyType& raw_key_value(const KeyType* keys, int index) {
         return keys[index];
      }

      inline const char* raw_key_value(const std::string* keys, int index) {
         return keys[index].data();
      }

      template<typename Type>
      void set_key(Type& a, std::add_const_t<Type>& b) {
         a = b;
      }

      inline void set_key(std::string& s, const shared_string& ss) {
         s.assign(ss.data(), ss.size());
      };

      inline void set_key(shared_string& s, const std::string& ss) {
         s.assign(ss.data(), ss.size());
      };

      template< typename ObjectType, int KeyIndex >
      struct key_helper_impl {
         using KeyType = typename ObjectType::key_type;
         using KeyAccess = object_key_value<KeyIndex>;
         using Next = key_helper_impl<ObjectType, KeyIndex - 1>;

         static void set(ObjectType& o, const KeyType* keys) {
            set_key(KeyAccess::get(o),*(keys + KeyIndex));
            Next::set(o,keys);
         }

         static void get(KeyType* keys, const ObjectType& o) {
            set_key(*(keys + KeyIndex),KeyAccess::get(o));
            Next::get(keys, o);
         }

         static bool compare(const ObjectType& o, const KeyType* keys) {
            return (KeyAccess::get(o) == raw_key_value(keys, KeyIndex)) && Next::compare(o, keys);
         }
      };

      template< typename ObjectType >
      struct key_helper_impl<ObjectType, -1> {
         using KeyType = typename ObjectType::key_type;
         static void set(ObjectType&, const KeyType*) {}
         static void get(KeyType*, const ObjectType&) {}
         static bool compare(const ObjectType&, const KeyType*) { return true; }
      };

      template< typename ObjectType >
      using key_helper = key_helper_impl<ObjectType, ObjectType::number_of_keys - 1>;

      template< typename KeyType, int KeyIndex, size_t Offset, typename ... Args >
      struct partial_tuple_impl {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return partial_tuple_impl<KeyType, KeyIndex - 1, Offset, KeyType, Args...>::get(tid, keys, raw_key_value(keys, Offset + KeyIndex), args...);
         }
      };

      template< typename KeyType, size_t Offset, typename ... Args >
      struct partial_tuple_impl<KeyType, 0, Offset, Args...> {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return boost::make_tuple( tid.id, raw_key_value(keys, Offset), args...);
         }
      };

      template< typename IndexType, typename Scope >
      using partial_tuple = partial_tuple_impl<typename IndexType::value_type::key_type, IndexType::value_type::number_of_keys - impl::scope_to_key_index_v<Scope> - 1, impl::scope_to_key_index_v<Scope>>;

      template <typename ObjectType>
      using exact_tuple = partial_tuple_impl<typename ObjectType::key_type, ObjectType::number_of_keys - 1, 0>;

      template <typename IndexType, typename Scope>
      struct record_scope_compare {
         using ObjectType = typename IndexType::value_type;
         using KeyType = typename ObjectType::key_type;
         static constexpr int KeyIndex = scope_to_key_index_v<Scope>;
         using KeyAccess = object_key_value<KeyIndex>;

         static bool compare(const ObjectType& o, const KeyType* keys) {
            return KeyAccess::get(o) == raw_key_value(keys, KeyIndex);
         }
      };

      template < typename IndexType, typename Scope >
      struct front_record_tuple {
         static auto get( const contracts::table_id_object& tid ) {
            return boost::make_tuple( tid.id );
         }
      };
   }



   template <typename ObjectType>
   int32_t apply_context::store_record( const table_id_object& t_id, const account_name& bta, const typename ObjectType::key_type* keys, const char* value, size_t valuelen ) {
      require_write_lock( t_id.scope );
      validate_or_add_table_key(t_id, get_key_type<ObjectType>());

      auto tuple = impl::exact_tuple<ObjectType>::get(t_id, keys);
      const auto* obj = db.find<ObjectType, contracts::by_scope_primary>(tuple);

      if( obj ) {
         mutable_db.modify( *obj, [&]( auto& o ) {
            o.value.assign(value, valuelen);
         });
         return 0;
      } else {
         mutable_db.create<ObjectType>( [&](auto& o) {
            o.t_id = t_id.id;
            impl::key_helper<ObjectType>::set(o, keys);
            o.value.insert( 0, value, valuelen );
         });
         return 1;
      }
   }

   template <typename ObjectType>
   int32_t apply_context::update_record( const table_id_object& t_id, const account_name& bta, const typename ObjectType::key_type* keys, const char* value, size_t valuelen ) {
      require_write_lock( t_id.scope );
      validate_or_add_table_key(t_id, get_key_type<ObjectType>());

      auto tuple = impl::exact_tuple<ObjectType>::get(t_id, keys);
      const auto* obj = db.find<ObjectType, contracts::by_scope_primary>(tuple);

      if( !obj ) {
         return 0;
      }

      mutable_db.modify( *obj, [&]( auto& o ) {
         if( valuelen > o.value.size() ) {
            o.value.resize(valuelen);
         }
         memcpy(o.value.data(), value, valuelen);
      });

      return 1;
   }

   template <typename ObjectType>
   int32_t apply_context::remove_record( const table_id_object& t_id, const typename ObjectType::key_type* keys ) {
      require_write_lock( t_id.scope );
      validate_or_add_table_key(t_id, get_key_type<ObjectType>());

      auto tuple = impl::exact_tuple<ObjectType>::get(t_id, keys);
      const auto* obj = db.find<ObjectType,  contracts::by_scope_primary>(tuple);
      if( obj ) {
         mutable_db.remove( *obj );
         return 1;
      }
      return 0;
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::load_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::partial_tuple<IndexType, Scope>::get(t_id, keys);
      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->t_id != t_id.id ||
          !impl::record_scope_compare<IndexType, Scope>::compare(*itr, keys)) return -1;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);
      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::front_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::front_record_tuple<IndexType, Scope>::get(t_id);

      auto itr = idx.lower_bound( tuple );
      if( itr == idx.end() ||
         itr->t_id != t_id.id ) return -1;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::back_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& idx = db.get_index<IndexType, Scope>();
      decltype(t_id.id) next_tid(t_id.id._id + 1);
      auto tuple = boost::make_tuple( next_tid );
      auto itr = idx.lower_bound(tuple);

      if( std::distance(idx.begin(), itr) == 0 ) return -1;

      --itr;

      if( itr->t_id != t_id.id ) return -1;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::next_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& pidx = db.get_index<IndexType, contracts::by_scope_primary>();

      auto tuple = impl::exact_tuple<typename IndexType::value_type>::get(t_id, keys);
      auto pitr = pidx.find(tuple);

      if(pitr == pidx.end())
        return -1;

      const auto& fidx = db.get_index<IndexType>();
      auto itr = fidx.indicies().template project<Scope>(pitr);

      const auto& idx = db.get_index<IndexType, Scope>();

      if( itr == idx.end() ||
          itr->t_id != t_id.id ||
          !impl::key_helper<typename IndexType::value_type>::compare(*itr, keys) ) {
        return -1;
      }

      ++itr;

      if( itr == idx.end() ||
          itr->t_id != t_id.id ) {
        return -1;
      }

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::previous_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& pidx = db.get_index<IndexType, contracts::by_scope_primary>();

      auto tuple = impl::exact_tuple<typename IndexType::value_type>::get(t_id, keys);
      auto pitr = pidx.find(tuple);

      if(pitr == pidx.end())
        return 0;

      const auto& fidx = db.get_index<IndexType>();
      auto itr = fidx.indicies().template project<Scope>(pitr);

      const auto& idx = db.get_index<IndexType, Scope>();

      if( itr == idx.end() ||
          itr == idx.begin() ||
          itr->t_id != t_id.id ||
          !impl::key_helper<typename IndexType::value_type>::compare(*itr, keys) ) return 0;

      --itr;

      if( itr->t_id != t_id.id ) return 0;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::lower_bound_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::partial_tuple<IndexType, Scope>::get(t_id, keys);
      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->t_id != t_id.id) return 0;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

   template <typename IndexType, typename Scope>
   int32_t apply_context::upper_bound_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );
      validate_table_key(t_id, get_key_type<typename IndexType::value_type>());

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::partial_tuple<IndexType, Scope>::get(t_id, keys);
      auto itr = idx.upper_bound(tuple);

      if( itr == idx.end() ||
          itr->t_id != t_id.id ) return 0;

      impl::key_helper<typename IndexType::value_type>::get(keys, *itr);

      if (valuelen) {
         auto copylen = std::min<size_t>(itr->value.size(), valuelen);
         if (copylen) {
            itr->value.copy(value, copylen);
         }
         return copylen;
      } else {
         return itr->value.size();
      }
   }

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::apply_context::apply_results, (applied_actions)(generated_transactions))
