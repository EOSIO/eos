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

namespace chainbase { class database; }

namespace eosio { namespace chain {

class chain_controller;

class apply_context {

   public:
      apply_context(chain_controller& con, chainbase::database& db, const action& a, const transaction_metadata& trx_meta)

      :controller(con), db(db), act(a), mutable_controller(con),
       mutable_db(db), used_authorizations(act.authorization.size(), false),
       trx_meta(trx_meta) {}

      void exec();

      void execute_inline( action &&a );
      void execute_deferred( deferred_transaction &&trx );
      void cancel_deferred( uint32_t sender_id );

      using table_id_object = contracts::table_id_object;
      const table_id_object* find_table( name scope, name code, name table );
      const table_id_object& find_or_create_table( name scope, name code, name table );

      template <typename ObjectType>
      int32_t store_record( const table_id_object& t_id, const typename ObjectType::key_type* keys, const char* value, size_t valuelen );

      template <typename ObjectType>
      int32_t update_record( const table_id_object& t_id, const typename ObjectType::key_type* keys, const char* value, size_t valuelen );

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

      const chain_controller&       controller;
      const chainbase::database&    db;  ///< database where state is stored
      const action&                 act; ///< message being applied
      account_name                  receiver; ///< the code that is currently running

      chain_controller&             mutable_controller;
      chainbase::database&          mutable_db;


      ///< Parallel to act.authorization; tracks which permissions have been used while processing the message
      vector<bool> used_authorizations;

      const transaction_metadata&   trx_meta;

   ///< pending transaction construction
     /*
      typedef uint32_t pending_transaction_handle;
      struct pending_transaction : public transaction {
         typedef uint32_t handle_type;
         
         pending_transaction(const handle_type& _handle, const apply_context& _context, const uint16_t& block_num, const uint32_t& block_ref, const time_point& expiration )
            : transaction(block_num, block_ref, expiration, vector<account_name>(),  vector<account_name>(), vector<action>())
            , handle(_handle)
            , context(_context) {}
         
         
         handle_type handle;
         const apply_context& context;

         void check_size() const;
      };

      pending_transaction::handle_type next_pending_transaction_serial;
      vector<pending_transaction> pending_transactions;

      pending_transaction& get_pending_transaction(pending_transaction::handle_type handle);
      pending_transaction& create_pending_transaction();
      void release_pending_transaction(pending_transaction::handle_type handle);

      ///< pending message construction
      typedef uint32_t pending_message_handle;
      struct pending_message : public action {
         typedef uint32_t handle_type;
         
         pending_message(const handle_type& _handle, const account_name& code, const action_name& type, const vector<char>& data)
            : action(code, type, vector<permission_level>(), data)
            , handle(_handle) {}

         handle_type handle;
      };

      pending_transaction::handle_type next_pending_message_serial;
      vector<pending_message> pending_messages;

      pending_message& get_pending_message(pending_message::handle_type handle);
      pending_message& create_pending_message(const account_name& code, const action_name& type, const vector<char>& data);
      void release_pending_message(pending_message::handle_type handle);
      */

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


   private:
      void append_results(apply_results &&other) {
         fc::move_append(results.applied_actions, move(other.applied_actions));
         fc::move_append(results.generated_transactions, move(other.generated_transactions));
         fc::move_append(results.canceled_deferred, move(other.canceled_deferred));
      }

      void exec_one();
   
      vector<account_name>                _notified; ///< keeps track of new accounts to be notifed of current message
      vector<action>                      _inline_actions; ///< queued inline messages
      std::ostringstream                  _pending_console_output;

      vector<shard_lock>                  _read_locks;
      vector<scope_name>                  _write_scopes;
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
            return o.primary_key;
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
            return o.primary_key;
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

      /// find_tuple helper
      template <typename KeyType, int KeyIndex, typename ...Args>
      struct exact_tuple_impl {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args ) {
            return exact_tuple_impl<KeyType, KeyIndex - 1,  const KeyType &, Args...>::get(tid, keys, raw_key_value(keys, KeyIndex), args...);
         }
      };

      template <typename KeyType, typename ...Args>
      struct exact_tuple_impl<KeyType, -1, Args...> {
         static auto get(const contracts::table_id_object& tid, const KeyType*, Args... args) {
            return boost::make_tuple(tid.id, args...);
         }
      };

      template <typename ObjectType>
      using exact_tuple = exact_tuple_impl<typename ObjectType::key_type, ObjectType::number_of_keys - 1>;

      template< typename KeyType, int NullKeyCount, typename Scope, typename ... Args >
      struct lower_bound_tuple_impl {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return lower_bound_tuple_impl<KeyType, NullKeyCount - 1, Scope, KeyType, Args...>::get(tid, keys, KeyType(0), args...);
         }
      };

      template< typename KeyType, typename Scope, typename ... Args >
      struct lower_bound_tuple_impl<KeyType, 0, Scope, Args...> {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return boost::make_tuple( tid.id, raw_key_value(keys, scope_to_key_index_v<Scope>), args...);
         }
      };

      template< typename IndexType, typename Scope >
      using lower_bound_tuple = lower_bound_tuple_impl<typename IndexType::value_type::key_type, IndexType::value_type::number_of_keys - scope_to_key_index_v<Scope> - 1, Scope>;

      template< typename KeyType, int NullKeyCount, typename Scope, typename ... Args >
      struct upper_bound_tuple_impl {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return upper_bound_tuple_impl<KeyType, NullKeyCount - 1, Scope, KeyType, Args...>::get(tid, keys, KeyType(-1), args...);
         }
      };

      template< typename KeyType, typename Scope, typename ... Args >
      struct upper_bound_tuple_impl<KeyType, 0, Scope, Args...> {
         static auto get(const contracts::table_id_object& tid, const KeyType* keys, Args... args) {
            return boost::make_tuple( tid.id, raw_key_value(keys, scope_to_key_index_v<Scope>), args...);
         }
      };

      template< typename IndexType, typename Scope >
      using upper_bound_tuple = upper_bound_tuple_impl<typename IndexType::value_type::key_type, IndexType::value_type::number_of_keys - scope_to_key_index_v<Scope> - 1, Scope>;

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
   int32_t apply_context::store_record( const table_id_object& t_id, const typename ObjectType::key_type* keys, const char* value, size_t valuelen ) {
      require_write_lock( t_id.scope );

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
   int32_t apply_context::update_record( const table_id_object& t_id, const typename ObjectType::key_type* keys, const char* value, size_t valuelen ) {
      require_write_lock( t_id.scope );
      
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

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::lower_bound_tuple<IndexType, Scope>::get(t_id, keys);
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

      const auto& pidx = db.get_index<IndexType, contracts::by_scope_primary>();
      
      auto tuple = impl::exact_tuple<typename IndexType::value_type>::get(t_id, keys);
      auto pitr = pidx.find(tuple);

      if(pitr == pidx.end())
        return -1;

      const auto& fidx = db.get_index<IndexType>();
      auto itr = fidx.indicies().template project<Scope>(pitr);

      const auto& idx = db.get_index<IndexType, Scope>();
      
      if( itr == idx.end() ||
          itr == idx.begin() ||
          itr->t_id != t_id.id ||
          !impl::key_helper<typename IndexType::value_type>::compare(*itr, keys) ) return -1;

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
   int32_t apply_context::lower_bound_record( const table_id_object& t_id, typename IndexType::value_type::key_type* keys, char* value, size_t valuelen ) {
      require_read_lock( t_id.code, t_id.scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::lower_bound_tuple<IndexType, Scope>::get(t_id, keys);
      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->t_id != t_id.id) return -1;

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

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = impl::upper_bound_tuple<IndexType, Scope>::get(t_id, keys);
      auto itr = idx.upper_bound(tuple);

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

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::apply_context::apply_results, (applied_actions)(generated_transactions));
