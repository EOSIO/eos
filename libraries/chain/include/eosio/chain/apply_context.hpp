#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_chainbase_iter_store.hpp>
#include <eosio/chain/backing_store/db_secondary_key_helper.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

namespace chainbase { class database; }

namespace eosio { namespace chain {

class controller;
class transaction_context;

class apply_context {
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

            using secondary_key_helper_t = backing_store::db_secondary_key_helper<secondary_key_type, secondary_key_proxy_type, secondary_key_proxy_const_type>;

            generic_index( apply_context& c ):context(c){}

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

               std::string event_id;
               context.db.modify( tab, [&]( auto& t ) {
                 ++t.count;

                  if (context.control.get_deep_mind_logger() != nullptr) {
                     event_id = backing_store::db_context::table_event(t.code, t.scope, t.table, name(id));
                  }
               });

               context.update_db_usage( payer, config::billable_size_v<ObjectType>, backing_store::db_context::secondary_add_trace(context.get_action_id(), std::move(event_id)) );

               itr_cache.cache_table( tab );
               return itr_cache.add( obj );
            }

            void remove( int iterator ) {
               const auto& obj = itr_cache.get( iterator );

               const auto& table_obj = itr_cache.get_table( obj.t_id );
               EOS_ASSERT( table_obj.code == context.receiver, table_access_violation, "db access violation" );

               std::string event_id;
               if (context.control.get_deep_mind_logger() != nullptr) {
                  event_id = backing_store::db_context::table_event(table_obj.code, table_obj.scope, table_obj.table, name(obj.primary_key));
               }

               context.update_db_usage( obj.payer, -( config::billable_size_v<ObjectType> ), backing_store::db_context::secondary_rem_trace(context.get_action_id(), std::move(event_id)) );

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

               std::string event_id;
               if (context.control.get_deep_mind_logger() != nullptr) {
                  event_id = backing_store::db_context::table_event(table_obj.code, table_obj.scope, table_obj.table, name(obj.primary_key));
               }

               if( obj.payer != payer ) {
                  context.update_db_usage( obj.payer, -(billing_size), backing_store::db_context::secondary_update_rem_trace(context.get_action_id(), std::string(event_id)) );
                  context.update_db_usage( payer, +(billing_size), backing_store::db_context::secondary_update_add_trace(context.get_action_id(), std::move(event_id)) );
               }

               context.db.modify( obj, [&]( auto& o ) {
                 secondary_key_helper_t::set(o.secondary_key, secondary);
                 o.payer = payer;
               });
            }

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

         private:
            apply_context&                                     context;
            backing_store::db_chainbase_iter_store<ObjectType> itr_cache;
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

   private:
      template <typename Exception>
      void check_unprivileged_resource_usage(const char* resource, const flat_set<account_delta>& deltas);

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
      void require_authorization(const account_name& account) const;
      bool has_authorization(const account_name& account) const;
      void require_authorization(const account_name& account, const permission_name& permission) const;

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

      void console_append( std::string_view val ) {
         _pending_console_output += val;
      }

   /// Database methods:
   public:

      void update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace );

      int  db_store_i64_chainbase( name scope, name table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size );
      void db_update_i64_chainbase( int iterator, account_name payer, const char* buffer, size_t buffer_size );
      void db_remove_i64_chainbase( int iterator );
      int  db_get_i64_chainbase( int iterator, char* buffer, size_t buffer_size );
      int  db_next_i64_chainbase( int iterator, uint64_t& primary );
      int  db_previous_i64_chainbase( int iterator, uint64_t& primary );
      int  db_find_i64_chainbase( name code, name scope, name table, uint64_t id );
      int  db_lowerbound_i64_chainbase( name code, name scope, name table, uint64_t id );
      int  db_upperbound_i64_chainbase( name code, name scope, name table, uint64_t id );
      int  db_end_i64_chainbase( name code, name scope, name table );

      backing_store::db_context& db_get_context();

   private:

      const table_id_object* find_table( name code, name scope, name table );
      const table_id_object& find_or_create_table( name code, name scope, name table, const account_name &payer );
      void                   remove_table( const table_id_object& tid );

   /// KV Database methods:
   public:
      int64_t  kv_erase(uint64_t contract, const char* key, uint32_t key_size);
      int64_t  kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size, account_name payer);
      bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size);
      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size);
      uint32_t kv_it_create(uint64_t contract, const char* prefix, uint32_t size);
      void     kv_it_destroy(uint32_t itr);
      int32_t  kv_it_status(uint32_t itr);
      int32_t  kv_it_compare(uint32_t itr_a, uint32_t itr_b);
      int32_t  kv_it_key_compare(uint32_t itr, const char* key, uint32_t size);
      int32_t  kv_it_move_to_end(uint32_t itr);
      int32_t  kv_it_next(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size);
      int32_t  kv_it_prev(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size);
      int32_t  kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size, uint32_t* found_key_size, uint32_t* found_value_size);
      int32_t  kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      int32_t  kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      kv_context& kv_get_backing_store() {
         EOS_ASSERT( kv_backing_store, action_validate_exception, "KV APIs cannot access state (null backing_store)" );
         return *kv_backing_store;
      }

   private:
      void kv_check_iterator(uint32_t itr);

   /// Misc methods:
   public:


      int get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const;
      int get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const;
      vector<account_name> get_active_producers() const;

      uint64_t next_global_sequence();
      uint64_t next_recv_sequence( const account_metadata_object& receiver_account );
      uint64_t next_auth_sequence( account_name actor );

      void add_ram_usage( account_name account, int64_t ram_delta, const storage_usage_trace& trace );

      void finalize_trace( action_trace& trace, const fc::time_point& start );

      bool is_context_free()const { return context_free; }
      bool is_privileged()const { return privileged; }
      action_name get_receiver()const { return receiver; }
      const action& get_action()const { return *act; }

      action_name get_sender() const;

      uint32_t get_action_id() const;
      void increment_action_id();

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

      std::unique_ptr<kv_context>                                    kv_backing_store;
      std::vector<std::unique_ptr<kv_iterator>>                      kv_iterators;
      std::vector<size_t>                                            kv_destroyed_iterators;

   private:

      backing_store::db_chainbase_iter_store<key_value_object> db_iter_store;
      vector< std::pair<account_name, uint32_t> >              _notified; ///< keeps track of new accounts to be notifed of current message
      vector<uint32_t>                                         _inline_actions; ///< action_ordinals of queued inline actions
      vector<uint32_t>                                         _cfa_inline_actions; ///< action_ordinals of queued inline context-free actions
      std::string                                              _pending_console_output;
      flat_set<account_delta>                                  _account_ram_deltas; ///< flat_set of account_delta so json is an array of objects

      std::unique_ptr<backing_store::db_context>               _db_context;
};

using apply_handler = std::function<void(apply_context&)>;

} } // namespace eosio::chain
