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
   try {
      auto native = mutable_controller.find_apply_handler(receiver, act.account, act.name);
      if (native) {
         (*native)(*this);
      } else {
         const auto &a = mutable_controller.get_database().get<account_object, by_name>(receiver);
         privileged = a.privileged;

         if (a.code.size() > 0) {
            // get code from cache
            auto code = mutable_controller.get_wasm_cache().checkout_scoped(a.code_version, a.code.data(),
                                                                            a.code.size());

            // get wasm_interface
            auto &wasm = wasm_interface::get();
            wasm.apply(code, *this);
         }
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

   results.applied_actions.emplace_back(action_trace {receiver, act, _pending_console_output.str(), 0, 0, move(data_access)});
   _pending_console_output = std::ostringstream();
   _read_locks.clear();
   _write_scopes.clear();
}

void apply_context::exec()
{
   _notified.push_back(act.account);

   for( uint32_t i = 0; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      exec_one();
   }

   for( uint32_t i = 0; i < _inline_actions.size(); ++i ) {
      apply_context ncontext( mutable_controller, mutable_db, _inline_actions[i], trx_meta);
      ncontext.exec();
      append_results(move(ncontext.results));
   }

} /// exec()

bool apply_context::is_account( const account_name& account )const {
   return nullptr != db.find<account_object,by_name>( account );
}

void apply_context::require_authorization( const account_name& account )const {
  for( const auto& auth : act.authorization )
     if( auth.actor == account ) return;
  wdump((act));
  EOS_ASSERT( false, tx_missing_auth, "missing authority of ${account}", ("account",account));
}
void apply_context::require_authorization(const account_name& account, 
                                          const permission_name& permission)const {
  for( const auto& auth : act.authorization )
     if( auth.actor == account ) {
        if( auth.permission == permission ) return;
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

void apply_context::execute_inline( action &&a ) {
   // todo: rethink this special case
   if (receiver != config::system_account_name) {
      controller.check_authorization({a}, flat_set<public_key_type>(), false, {receiver});
   }
   _inline_actions.emplace_back( move(a) );
}

void apply_context::execute_deferred( deferred_transaction&& trx ) {
   try {
      FC_ASSERT( trx.expiration > (controller.head_block_time() + fc::milliseconds(2*config::block_interval_ms)),
                                   "transaction is expired when created" );

      FC_ASSERT( trx.execute_after < trx.expiration, "transaction expires before it can execute" );

      /// TODO: make default_max_gen_trx_count a producer parameter
      FC_ASSERT( results.generated_transactions.size() < config::default_max_gen_trx_count );

      FC_ASSERT( !trx.actions.empty(), "transaction must have at least one action");

      // todo: rethink this special case
      if (receiver != config::system_account_name) {
         controller.check_authorization(trx.actions, flat_set<public_key_type>(), false, {receiver});
      }

      trx.sender = receiver; //  "Attempting to send from another account"
      trx.set_reference_block(controller.head_block_id());

      /// TODO: make sure there isn't already a deferred transaction with this ID or senderID?
      results.generated_transactions.emplace_back(move(trx));
   } FC_CAPTURE_AND_RETHROW((trx));
}

void apply_context::cancel_deferred( uint32_t sender_id ) {
   results.canceled_deferred.emplace_back(receiver, sender_id);
}

const contracts::table_id_object* apply_context::find_table( name code, name scope, name table ) {
   require_read_lock(code, scope);
   return db.find<table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, scope, table));
}

const contracts::table_id_object& apply_context::find_or_create_table( name code, name scope, name table ) {
   require_read_lock(code, scope);
   const auto* existing_tid =  db.find<contracts::table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, scope, table));
   if (existing_tid != nullptr) {
      return *existing_tid;
   }

   require_write_lock(scope);
   return mutable_db.create<contracts::table_id_object>([&](contracts::table_id_object &t_id){
      t_id.code = code;
      t_id.scope = scope;
      t_id.table = table;
   });
}

vector<account_name> apply_context::get_active_producers() const {
   const auto& gpo = controller.get_global_properties();
   vector<account_name> accounts;
   for(const auto& producer : gpo.active_producers.producers)
      accounts.push_back(producer.producer_name);

   return accounts;
}

void apply_context::checktime(uint32_t instruction_count) const {
   if (trx_meta.processing_deadline && fc::time_point::now() > (*trx_meta.processing_deadline)) {
      throw checktime_exceeded();
   }
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

const char* to_string(contracts::table_key_type key_type) {
   switch(key_type) {
   case contracts::type_unassigned:
      return "unassigned";
   case contracts::type_i64:
      return "i64";
   case contracts::type_str:
      return "str";
   case contracts::type_i128i128:
      return "i128i128";
   case contracts::type_i64i64:
      return "i64i64";
   case contracts::type_i64i64i64:
      return "i64i64i64";
   default:
      return "<unkown table_key_type>";
   }
}

void apply_context::validate_table_key( const table_id_object& t_id, contracts::table_key_type key_type ) {
   FC_ASSERT( t_id.key_type == contracts::table_key_type::type_unassigned || key_type == t_id.key_type,
              "Table entry for ${code}-${scope}-${table} uses key type ${act_type} should have had type of ${exp_type}",
              ("code",t_id.code)("scope",t_id.scope)("table",t_id.table)("act_type",to_string(t_id.key_type))("exp_type", to_string(key_type)) );
}

void apply_context::validate_or_add_table_key( const table_id_object& t_id, contracts::table_key_type key_type ) {
   if (t_id.key_type == contracts::table_key_type::type_unassigned)
      mutable_db.modify( t_id, [&key_type]( auto& o) {
         o.key_type = key_type;
      });
   else
      FC_ASSERT( key_type == t_id.key_type,
                 "Table entry for ${code}-${scope}-${table} uses key type ${act_type} should have had type of ${exp_type}",
                 ("code",t_id.code)("scope",t_id.scope)("table",t_id.table)("act_type",to_string(t_id.key_type))("exp_type", to_string(key_type)) );
}

void apply_context::update_db_usage( const account_name& payer, int64_t delta ) {
   require_write_lock( payer );
   if( (delta > 0) && payer != account_name(receiver) ) {
      require_authorization( payer );
   }
}


int apply_context::db_store_i64( uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size ) {
   require_write_lock( scope );
   const auto& tab = find_or_create_table( receiver, scope, table );
   auto tableid = tab.id;
   validate_or_add_table_key(tab, contracts::table_key_type::type_i64);

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

   update_db_usage( payer, buffer_size + 200 );

   keyval_cache.cache_table( tab );
   return keyval_cache.add( obj );
}

void apply_context::db_update_i64( int iterator, account_name payer, const char* buffer, size_t buffer_size ) {
   const key_value_object& obj = keyval_cache.get( iterator );

   require_write_lock( keyval_cache.get_table( obj.t_id ).scope );

   int64_t old_size = obj.value.size();

   if( payer == account_name() ) payer = obj.payer;

   if( account_name(obj.payer) == payer ) {
      update_db_usage( obj.payer, buffer_size + 200 - old_size );
   } else  {
      update_db_usage( obj.payer,  -(old_size+200) );
      update_db_usage( payer,  (buffer_size+200) );
   }

   mutable_db.modify( obj, [&]( auto& o ) {
     o.value.resize( buffer_size );
     memcpy( o.value.data(), buffer, buffer_size );
     o.payer = payer;
   });
}

void apply_context::db_remove_i64( int iterator ) {
   const key_value_object& obj = keyval_cache.get( iterator );
   update_db_usage( obj.payer,  -(obj.value.size()+200) );

   const auto& table_obj = keyval_cache.get_table( obj.t_id );
   require_write_lock( table_obj.scope );

   mutable_db.modify( table_obj, [&]( auto& t ) {
      --t.count;
   });
   mutable_db.remove( obj );

   keyval_cache.remove( iterator, obj );
}

int apply_context::db_get_i64( int iterator, char* buffer, size_t buffer_size ) {
   const key_value_object& obj = keyval_cache.get( iterator );
   if( buffer_size >= obj.value.size() )
      memcpy( buffer, obj.value.data(), obj.value.size() );
   
   return obj.value.size();
}

int apply_context::db_next_i64( int iterator, uint64_t& primary ) {
   const auto& obj = keyval_cache.get( iterator );
   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();

   auto itr = idx.iterator_to( obj );
   ++itr;

   if( itr == idx.end() ) return -1;
   if( itr->t_id != obj.t_id ) return -1;

   primary = itr->primary_key;
   return keyval_cache.add( *itr );
}

int apply_context::db_previous_i64( int iterator, uint64_t& primary ) {
   const auto& obj = keyval_cache.get(iterator);
   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();
   
   auto itr = idx.iterator_to(obj);
   if (itr == idx.end() || itr == idx.begin()) return -1;

   --itr;

   if (itr->t_id != obj.t_id) return -1;
   
   primary = itr->primary_key;
   return keyval_cache.add(*itr);
}

int apply_context::db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope );

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;
   validate_table_key(*tab, contracts::table_key_type::type_i64);


   const key_value_object* obj = db.find<key_value_object, contracts::by_scope_primary>( boost::make_tuple( tab->id, id ) );
   if( !obj ) return -1;

   keyval_cache.cache_table( *tab );
   return keyval_cache.add( *obj );
}

int apply_context::db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope );

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;
   validate_table_key(*tab, contracts::table_key_type::type_i64);


   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();
   auto itr = idx.lower_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return -1;
   if( itr->t_id != tab->id ) return -1;

   keyval_cache.cache_table( *tab );
   return keyval_cache.add( *itr );
}

int apply_context::db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   require_read_lock( code, scope );

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;
   validate_table_key(*tab, contracts::table_key_type::type_i64);


   const auto& idx = db.get_index<contracts::key_value_index, contracts::by_scope_primary>();
   auto itr = idx.upper_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return -1;
   if( itr->t_id != tab->id ) return -1;

   keyval_cache.cache_table( *tab );
   return keyval_cache.add( *itr );
}

template<>
contracts::table_key_type apply_context::get_key_type<contracts::key_value_object>() {
   return contracts::table_key_type::type_i64;
}

template<>
contracts::table_key_type apply_context::get_key_type<contracts::keystr_value_object>() {
   return contracts::table_key_type::type_str;
}

template<>
contracts::table_key_type apply_context::get_key_type<contracts::key128x128_value_object>() {
   return contracts::table_key_type::type_i128i128;
}

template<>
contracts::table_key_type apply_context::get_key_type<contracts::key64x64_value_object>() {
   return contracts::table_key_type::type_i64i64;
}

template<>
contracts::table_key_type apply_context::get_key_type<contracts::key64x64x64_value_object>() {
   return contracts::table_key_type::type_i64i64i64;
}
} } /// eosio::chain
