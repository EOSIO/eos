#include <algorithm>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace eosio { namespace chain {
void apply_context::exec_one()
{
   auto start = fc::time_point::now();
   _cpu_usage = 0;
   try {
      const auto &a = mutable_controller.get_database().get<account_object, by_name>(receiver);
      privileged = a.privileged;
      auto native = mutable_controller.find_apply_handler(receiver, act.account, act.name);
      if (native) {
         (*native)(*this);
      }

      if (a.code.size() > 0 && !(act.name == N(setcode) && act.account == config::system_account_name)) {
         try {
            mutable_controller.get_wasm_interface().apply(a.code_version, a.code, *this);
         } catch ( const wasm_exit& ){}
      }

   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

   if (!_write_scopes.empty()) {
      std::sort(_write_scopes.begin(), _write_scopes.end());
   }

   if (!_read_locks.empty()) {
      std::sort(_read_locks.begin(), _read_locks.end());
      // remove any write_scopes
      auto r_iter = _read_locks.begin();
      for( auto w_iter = _write_scopes.cbegin(); (w_iter != _write_scopes.cend()) && (r_iter != _read_locks.end()); ++w_iter) {
         shard_lock w_lock = {receiver, *w_iter};
         while(r_iter != _read_locks.end() && *r_iter < w_lock ) {
            ++r_iter;
         }

         if (*r_iter == w_lock) {
            r_iter = _read_locks.erase(r_iter);
         }
      }
   }

   // create a receipt for this
   vector<data_access_info> data_access;
   data_access.reserve(_write_scopes.size() + _read_locks.size());
   for (const auto& scope: _write_scopes) {
      auto key = boost::make_tuple(scope, receiver);
      const auto& scope_sequence = mutable_controller.get_database().find<scope_sequence_object, by_scope_receiver>(key);
      if (scope_sequence == nullptr) {
         try {
            mutable_controller.get_mutable_database().create<scope_sequence_object>([&](scope_sequence_object &ss) {
               ss.scope = scope;
               ss.receiver = receiver;
               ss.sequence = 1;
            });
         } FC_CAPTURE_AND_RETHROW((scope)(receiver));
         data_access.emplace_back(data_access_info{data_access_info::write, receiver, scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::write, receiver, scope, scope_sequence->sequence});
         try {
            mutable_controller.get_mutable_database().modify(*scope_sequence, [&](scope_sequence_object& ss) {
               ss.sequence += 1;
            });
         } FC_CAPTURE_AND_RETHROW((scope)(receiver));
      }
   }

   for (const auto& lock: _read_locks) {
      auto key = boost::make_tuple(lock.scope, lock.account);
      const auto& scope_sequence = mutable_controller.get_database().find<scope_sequence_object, by_scope_receiver>(key);
      if (scope_sequence == nullptr) {
         data_access.emplace_back(data_access_info{data_access_info::read, lock.account, lock.scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::read, lock.account, lock.scope, scope_sequence->sequence});
      }
   }

   std::cout << "Total CPU: " << _cpu_usage << std::endl;

   results.applied_actions.emplace_back(action_trace {receiver, context_free, _cpu_usage, act, _pending_console_output.str(), move(data_access)});
   reset_console();
   _read_locks.clear();
   _write_scopes.clear();
   results.applied_actions.back()._profiling_us = fc::time_point::now() - start;
}

void apply_context::exec()
{
   _notified.push_back(act.account);
   for( uint32_t i = 0; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      exec_one();
   }

   for( uint32_t i = 0; i < _cfa_inline_actions.size(); ++i ) {
      EOS_ASSERT( recurse_depth < config::max_recursion_depth, transaction_exception, "inline action recursion depth reached" );
      apply_context ncontext( mutable_controller, mutable_db, _cfa_inline_actions[i], trx_meta, recurse_depth + 1 );
      ncontext.context_free = true;
      ncontext.exec();
      append_results(move(ncontext.results));
   }

   for( uint32_t i = 0; i < _inline_actions.size(); ++i ) {
      EOS_ASSERT( recurse_depth < config::max_recursion_depth, transaction_exception, "inline action recursion depth reached" );
      apply_context ncontext( mutable_controller, mutable_db, _inline_actions[i], trx_meta, recurse_depth + 1 );
      ncontext.exec();
      append_results(move(ncontext.results));
   }

} /// exec()

bool apply_context::is_account( const account_name& account )const {
   return nullptr != db.find<account_object,by_name>( account );
}

bool apply_context::all_authorizations_used()const {
   for ( bool has_auth : used_authorizations ) {
      if ( !has_auth )
         return false;
   }
   return true;
}

vector<permission_level> apply_context::unused_authorizations()const {
   vector<permission_level> ret_auths;
   for ( uint32_t i=0; i < act.authorization.size(); i++ )
      if ( !used_authorizations[i] )
         ret_auths.push_back( act.authorization[i] );
   return ret_auths;
}

void apply_context::require_authorization( const account_name& account ) {
   for( uint32_t i=0; i < act.authorization.size(); i++ ) {
     if( act.authorization[i].actor == account ) {
        used_authorizations[i] = true;
        return;
     }
   }
   EOS_ASSERT( false, tx_missing_auth, "missing authority of ${account}", ("account",account));
}

bool apply_context::has_authorization( const account_name& account )const {
   for( const auto& auth : act.authorization )
     if( auth.actor == account )
        return true;
  return false;
}

void apply_context::require_authorization(const account_name& account,
                                          const permission_name& permission) {
  for( uint32_t i=0; i < act.authorization.size(); i++ )
     if( act.authorization[i].actor == account ) {
        if( act.authorization[i].permission == permission ) {
           used_authorizations[i] = true;
           return;
        }
     }
  EOS_ASSERT( false, tx_missing_auth, "missing authority of ${account}/${permission}",
              ("account",account)("permission",permission) );
}

static bool scopes_contain(const vector<scope_name>& scopes, const scope_name& scope) {
   return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
}

static bool locks_contain(const vector<shard_lock>& locks, const account_name& account, const scope_name& scope) {
   return std::find(locks.begin(), locks.end(), shard_lock{account, scope}) != locks.end();
}

void apply_context::require_write_lock(const scope_name& scope) {
   if (trx_meta.allowed_write_locks) {
      EOS_ASSERT( locks_contain(**trx_meta.allowed_write_locks, receiver, scope), block_lock_exception, "write lock \"${a}::${s}\" required but not provided", ("a", receiver)("s",scope) );
   }

   if (!scopes_contain(_write_scopes, scope)) {
      _write_scopes.emplace_back(scope);
   }
}

void apply_context::require_read_lock(const account_name& account, const scope_name& scope) {
   if (trx_meta.allowed_read_locks || trx_meta.allowed_write_locks ) {
      bool locked_for_read = trx_meta.allowed_read_locks && locks_contain(**trx_meta.allowed_read_locks, account, scope);
      if (!locked_for_read && trx_meta.allowed_write_locks) {
         locked_for_read = locks_contain(**trx_meta.allowed_write_locks, account, scope);
      }
      EOS_ASSERT( locked_for_read , block_lock_exception, "read lock \"${a}::${s}\" required but not provided", ("a", account)("s",scope) );
   }

   if (!locks_contain(_read_locks, account, scope)) {
      _read_locks.emplace_back(shard_lock{account, scope});
   }
}

bool apply_context::has_recipient( account_name code )const {
   for( auto a : _notified )
      if( a == code )
         return true;
   return false;
}

void apply_context::require_recipient( account_name code ) {
   if( !has_recipient(code) )
      _notified.push_back(code);
}


/**
 *  This will execute an action after checking the authorization. Inline transactions are
 *  implicitly authorized by the current receiver (running code). This method has significant
 *  security considerations and several options have been considered:
 *
 *  1. priviledged accounts (those marked as such by block producers) can authorize any action
 *  2. all other actions are only authorized by 'receiver' which means the following:
 *         a. the user must set permissions on their account to allow the 'receiver' to act on their behalf
 *
 *  Discarded Implemenation:  at one point we allowed any account that authorized the current transaction
 *   to implicitly authorize an inline transaction. This approach would allow privelege escalation and
 *   make it unsafe for users to interact with certain contracts.  We opted instead to have applications
 *   ask the user for permission to take certain actions rather than making it implicit. This way users
 *   can better understand the security risk.
 */
void apply_context::execute_inline( action&& a ) {
   if ( !privileged ) {
      if( a.account != receiver ) {
         const auto delay = controller.check_authorization({a}, flat_set<public_key_type>(), false, {receiver});
         FC_ASSERT( trx_meta.published + delay <= controller.head_block_time(),
                    "inline action uses a permission that imposes a delay that is not met, set delay_sec in transaction header to at least ${delay} seconds",
                    ("delay", delay.to_seconds()) );
      }
   }
   _inline_actions.emplace_back( move(a) );
}

void apply_context::execute_context_free_inline( action&& a ) {
   FC_ASSERT( a.authorization.size() == 0, "context free actions cannot have authorizations" );
   _cfa_inline_actions.emplace_back( move(a) );
}

void apply_context::execute_deferred( deferred_transaction&& trx ) {
   try {
      trx.set_reference_block(controller.head_block_id()); // No TaPoS check necessary
      trx.sender = receiver;
      controller.validate_transaction_without_state(trx);
      // transaction_api::send_deferred guarantees that trx.execute_after is at least head block time, so no need to check expiration.
      // Any other called of this function needs to similarly meet that precondition.
      EOS_ASSERT( trx.execute_after < trx.expiration,
                  transaction_exception,
                  "Transaction expires at ${trx.expiration} which is before the first allowed time to execute at ${trx.execute_after}",
                  ("trx.expiration",trx.expiration)("trx.execute_after",trx.execute_after) );

      controller.validate_expiration_not_too_far(trx, trx.execute_after);
      controller.validate_referenced_accounts(trx);

      controller.validate_uniqueness(trx); // TODO: Move this out of here when we have concurrent shards to somewhere we can check for conflicts between shards.

      const auto& gpo = controller.get_global_properties();
      FC_ASSERT( results.deferred_transactions_count < gpo.configuration.max_generated_transaction_count );

      fc::microseconds delay;

      // privileged accounts can do anything, no need to check auth
      if( !privileged ) {
         // check to make sure the payer has authorized this deferred transaction's storage in RAM
         if (trx.payer != receiver) {
            require_authorization(trx.payer);
         }

         if (trx.payer != receiver) {
            require_authorization(trx.payer);
         }

         // if a contract is deferring only actions to itself then there is no need
         // to check permissions, it could have done everything anyway.
         bool check_auth = false;
         for( const auto& act : trx.actions ) {
            if( act.account != receiver ) {
               check_auth = true;
               break;
            }
         }
         if( check_auth ) {
            delay = controller.check_authorization(trx.actions, flat_set<public_key_type>(), false, {receiver});
            FC_ASSERT( trx_meta.published + delay <= controller.head_block_time(),
                       "deferred transaction uses a permission that imposes a delay that is not met, set delay_sec in transaction header to at least ${delay} seconds",
                       ("delay", delay.to_seconds()) );
         }
      }

      auto now = controller.head_block_time();
      if( delay.count() ) {
         auto min_execute_after_time = time_point_sec(now + delay + fc::microseconds(999'999)); // rounds up nearest second
         EOS_ASSERT( min_execute_after_time <= trx.execute_after,
                     transaction_exception,
                     "deferred transaction is specified to execute after ${trx.execute_after} which is earlier than the earliest time allowed by authorization checker",
                     ("trx.execute_after",trx.execute_after)("min_execute_after_time",min_execute_after_time) );
      }

      results.deferred_transaction_requests.push_back(move(trx));
      results.deferred_transactions_count++;
   } FC_CAPTURE_AND_RETHROW((trx));
}

void apply_context::cancel_deferred( const uint128_t& sender_id ) {
   results.deferred_transaction_requests.push_back(deferred_reference(receiver, sender_id));
}

void apply_context::add_cpu_usage( const uint64_t usage ) {
   // TODO for now just increase the usage, in the future check against some limit
   _cpu_usage += usage;
}

const contracts::table_id_object* apply_context::find_table( name code, name scope, name table ) {
   require_read_lock(code, scope);
   return db.find<table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, scope, table));
}

const contracts::table_id_object& apply_context::find_or_create_table( name code, name scope, name table, const account_name &payer ) {
   require_read_lock(code, scope);
   const auto* existing_tid =  db.find<contracts::table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, scope, table));
   if (existing_tid != nullptr) {
      return *existing_tid;
   }

   require_write_lock(scope);

   update_db_usage(payer, config::billable_size_v<contracts::table_id_object>);

   return mutable_db.create<contracts::table_id_object>([&](contracts::table_id_object &t_id){
      t_id.code = code;
      t_id.scope = scope;
      t_id.table = table;
      t_id.payer = payer;
   });
}

void apply_context::remove_table( const contracts::table_id_object& tid ) {
   update_db_usage(tid.payer, - config::billable_size_v<contracts::table_id_object>);
   mutable_db.remove(tid);
}

vector<account_name> apply_context::get_active_producers() const {
   const auto& gpo = controller.get_global_properties();
   vector<account_name> accounts;
   for(const auto& producer : gpo.active_producers.producers)
      accounts.push_back(producer.producer_name);

   return accounts;
}

void apply_context::reset_console() {
   _pending_console_output = std::ostringstream();
   _pending_console_output.setf( std::ios::scientific, std::ios::floatfield );
}

void apply_context::checktime(uint32_t instruction_count) {
   if (trx_meta.processing_deadline && fc::time_point::now() > (*trx_meta.processing_deadline)) {
      throw checktime_exceeded();
   }
   _cpu_usage += instruction_count;
}


const bytes& apply_context::get_packed_transaction() {
   if( !trx_meta.packed_trx.size() ) {
      if (_cached_trx.empty()) {
         auto size = fc::raw::pack_size(trx_meta.trx());
         _cached_trx.resize(size);
         fc::datastream<char *> ds(_cached_trx.data(), size);
         fc::raw::pack(ds, trx_meta.trx());
      }

      return _cached_trx;
   }

   return trx_meta.packed_trx;
}

void apply_context::update_db_usage( const account_name& payer, int64_t delta ) {
   require_write_lock( payer );
   if( (delta > 0) ) {
      if (!(privileged || payer == account_name(receiver))) {
         require_authorization( payer );
      }

      mutable_controller.get_mutable_resource_limits_manager().add_pending_account_ram_usage(payer, delta);
   }
}


int apply_context::get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const
{
   const transaction& trx = trx_meta.trx();
   const action* act = nullptr;
   if( type == 0 ) {
      if( index >= trx.context_free_actions.size() )
         return -1;
      act = &trx.context_free_actions[index];
   }
   else if( type == 1 ) {
      if( index >= trx.actions.size() )
         return -1;
      act = &trx.actions[index];
   }
   else if( type == 2 ) {
      if( index >= _cfa_inline_actions.size() )
         return -1;
      act = &_cfa_inline_actions[index];
   }
   else if( type == 3 ) {
      if( index >= _inline_actions.size() )
         return -1;
      act = &_inline_actions[index];
   }

   auto ps = fc::raw::pack_size( *act );
   if( ps <= buffer_size ) {
      fc::datastream<char*> ds(buffer, buffer_size);
      fc::raw::pack( ds, *act );
   }
   return ps;
}

int apply_context::get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const {
   if( index >= trx_meta.context_free_data.size() ) return -1;

   auto s = trx_meta.context_free_data[index].size();

   if( buffer_size == 0 ) return s;

   if( buffer_size < s )
      memcpy( buffer, trx_meta.context_free_data[index].data(), buffer_size );
   else
      memcpy( buffer, trx_meta.context_free_data[index].data(), s );

   return s;
}

void apply_context::check_auth( const transaction& trx, const vector<permission_level>& perm ) {
   controller.check_authorization( trx.actions,
                                   {},
                                   true,
                                   {},
                                   flat_set<permission_level>(perm.begin(), perm.end()) );
}

int apply_context::db_store_i64( uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size ) {
   return db_store_i64( receiver, scope, table, payer, id, buffer, buffer_size);
}

int apply_context::db_store_i64( uint64_t code, uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size ) {
   require_write_lock( scope );
   const auto& tab = find_or_create_table( code, scope, table, payer );
   auto tableid = tab.id;

   FC_ASSERT( payer != account_name(), "must specify a valid account to pay for new record" );

   const auto& obj = mutable_db.create<key_value_object>( [&]( auto& o ) {
      o.t_id        = tableid;
      o.primary_key = id;
      o.value.resize( buffer_size );
      o.payer       = payer;
      memcpy( o.value.data(), buffer, buffer_size );
   });

   mutable_db.modify( tab, [&]( auto& t ) {
     ++t.count;
   });

   int64_t billable_size = (int64_t)(buffer_size + config::billable_size_v<key_value_object>);
   update_db_usage( payer, billable_size);

   keyval_cache.cache_table( tab );
   return keyval_cache.add( obj );
}

void apply_context::db_update_i64( int iterator, account_name payer, const char* buffer, size_t buffer_size ) {
   const key_value_object& obj = keyval_cache.get( iterator );

   const auto& table_obj = keyval_cache.get_table( obj.t_id );
   FC_ASSERT( table_obj.code == receiver, "db access violation" );

   require_write_lock( table_obj.scope );

   const int64_t overhead = config::billable_size_v<key_value_object>;
   int64_t old_size = (int64_t)(obj.value.size() + overhead);
   int64_t new_size = (int64_t)(buffer_size + overhead);

   if( payer == account_name() ) payer = obj.payer;

   if( account_name(obj.payer) != payer ) {
      // refund the existing payer
      update_db_usage( obj.payer,  -(old_size) );
      // charge the new payer
      update_db_usage( payer,  (new_size));
   } else if(old_size != new_size) {
      // charge/refund the existing payer the difference
      update_db_usage( obj.payer, new_size - old_size);
   }

   mutable_db.modify( obj, [&]( auto& o ) {
     o.value.resize( buffer_size );
     memcpy( o.value.data(), buffer, buffer_size );
     o.payer = payer;
   });
}

void apply_context::db_remove_i64( int iterator ) {
   const key_value_object& obj = keyval_cache.get( iterator );

   const auto& table_obj = keyval_cache.get_table( obj.t_id );
   FC_ASSERT( table_obj.code == receiver, "db access violation" );

   require_write_lock( table_obj.scope );

   update_db_usage( obj.payer,  -(obj.value.size() + config::billable_size_v<key_value_object>) );

   mutable_db.modify( table_obj, [&]( auto& t ) {
      --t.count;
   });
   mutable_db.remove( obj );

   if (table_obj.count == 0) {
      remove_table(table_obj);
   }

   keyval_cache.remove( iterator );
}

int apply_context::db_get_i64( int iterator, char* buffer, size_t buffer_size ) {
   const key_value_object& obj = keyval_cache.get( iterator );
   memcpy( buffer, obj.value.data(), std::min(obj.value.size(), buffer_size) );

   return obj.value.size();
}

int apply_context::db_next_i64( int iterator, uint64_t& primary ) {
   if( iterator < -1 ) return -1; // cannot increment past end iterator of table

   const auto& obj = keyval_cache.get( iterator ); // Check for iterator != -1 happens in this call
   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();

   auto itr = idx.iterator_to( obj );
   ++itr;

   if( itr == idx.end() || itr->t_id != obj.t_id ) return keyval_cache.get_end_iterator_by_table_id(obj.t_id);

   primary = itr->primary_key;
   return keyval_cache.add( *itr );
}

int apply_context::db_previous_i64( int iterator, uint64_t& primary ) {
   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();

   if( iterator < -1 ) // is end iterator
   {
      auto tab = keyval_cache.find_table_by_end_iterator(iterator);
      FC_ASSERT( tab, "not a valid end iterator" );

      auto itr = idx.upper_bound(tab->id);
      if( idx.begin() == idx.end() || itr == idx.begin() ) return -1; // Empty table

      --itr;

      if( itr->t_id != tab->id ) return -1; // Empty table

      primary = itr->primary_key;
      return keyval_cache.add(*itr);
   }

   const auto& obj = keyval_cache.get(iterator); // Check for iterator != -1 happens in this call

   auto itr = idx.iterator_to(obj);
   if( itr == idx.begin() ) return -1; // cannot decrement past beginning iterator of table

   --itr;

   if( itr->t_id != obj.t_id ) return -1; // cannot decrement past beginning iterator of table

   primary = itr->primary_key;
   return keyval_cache.add(*itr);
}

int apply_context::db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const key_value_object* obj = db.find<key_value_object, contracts::by_scope_primary>( boost::make_tuple( tab->id, id ) );
   if( !obj ) return table_end_itr;

   return keyval_cache.add( *obj );
}

int apply_context::db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();
   auto itr = idx.lower_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return table_end_itr;
   if( itr->t_id != tab->id ) return table_end_itr;

   return keyval_cache.add( *itr );
}

int apply_context::db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();
   auto itr = idx.upper_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return table_end_itr;
   if( itr->t_id != tab->id ) return table_end_itr;

   return keyval_cache.add( *itr );
}

int apply_context::db_end_i64( uint64_t code, uint64_t scope, uint64_t table ) {
   require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   return keyval_cache.cache_table( *tab );
}
} } /// eosio::chain
