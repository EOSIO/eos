#include <algorithm>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace eosio { namespace chain {
action_trace apply_context::exec_one()
{
   const auto& gpo = control.get_global_properties();

   auto start = fc::time_point::now();
   cpu_usage = gpo.configuration.base_per_action_cpu_usage;
   try {
      const auto &a = control.get_account(receiver);
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


   action_receipt r;
   r.receiver        = receiver;
   r.act_digest      = digest_type::hash(act);
   r.global_sequence = next_global_sequence();
   r.recv_sequence   = next_recv_sequence( receiver );

   for( const auto& auth : act.authorization ) {
      r.auth_sequence[auth.actor] = next_auth_sequence( auth.actor );
   }

   action_trace t(r);
   t.act = act;
   t.cpu_usage = cpu_usage;
   t.total_inline_cpu_usage = cpu_usage;
   t.console = _pending_console_output.str();

   executed.emplace_back( move(r) );
   total_cpu_usage += cpu_usage;

   _pending_console_output = std::ostringstream();

   t.elapsed = fc::time_point::now() - start;
   return t;
}

void apply_context::exec()
{
   _notified.push_back(act.account);
   for( uint32_t i = 0; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      if( i == 0 ) { /// first one is the root, of this trace
         trace = exec_one();
      } else {
         trace.inline_traces.emplace_back( exec_one() );
      }
   }

   for( uint32_t i = 0; i < _cfa_inline_actions.size(); ++i ) {
      EOS_ASSERT( recurse_depth < config::max_recursion_depth, transaction_exception, "inline action recursion depth reached" );
      apply_context ncontext( mutable_controller, _cfa_inline_actions[i], trx, recurse_depth + 1 );
      ncontext.context_free = true;
      ncontext.id = id;

      ncontext.processing_deadline = processing_deadline;
      ncontext.published_time      = published_time;
      ncontext.exec();
      fc::move_append( executed, move(ncontext.executed) );
      total_cpu_usage += ncontext.total_cpu_usage;

      trace.total_inline_cpu_usage += ncontext.trace.total_inline_cpu_usage;
      trace.inline_traces.emplace_back(ncontext.trace);
   }

   for( uint32_t i = 0; i < _inline_actions.size(); ++i ) {
      EOS_ASSERT( recurse_depth < config::max_recursion_depth, transaction_exception, "inline action recursion depth reached" );
      apply_context ncontext( mutable_controller, _inline_actions[i], trx, recurse_depth + 1 );
      ncontext.processing_deadline = processing_deadline;
      ncontext.published_time      = published_time;
      ncontext.id = id;
      ncontext.exec();
      fc::move_append( executed, move(ncontext.executed) );
      total_cpu_usage += ncontext.total_cpu_usage;

      trace.total_inline_cpu_usage += ncontext.trace.total_inline_cpu_usage;
      trace.inline_traces.emplace_back(ncontext.trace);
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

bool apply_context::has_recipient( account_name code )const {
   for( auto a : _notified )
      if( a == code )
         return true;
   return false;
}

void apply_context::require_recipient( account_name recipient ) {
   if( !has_recipient(recipient) ) {
      _notified.push_back(recipient);
   }
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
         const auto delay = control.get_authorization_manager().check_authorization({a}, flat_set<public_key_type>(), false, {receiver});
         FC_ASSERT( published_time + delay <= control.pending_block_time(),
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


/// TODO: rename this schedule_deferred it is not actually executed here
void apply_context::schedule_deferred_transaction( const uint128_t& sender_id, account_name payer, transaction&& trx ) {
   trx.set_reference_block(control.head_block_id()); // No TaPoS check necessary

   control.validate_referenced_accounts( trx );
   control.validate_expiration( trx );

   fc::microseconds required_delay;

   if( !privileged ) {
      if (payer != receiver) {
         require_authorization(payer); /// uses payer's storage
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
         required_delay = control.get_authorization_manager().check_authorization( trx.actions, flat_set<public_key_type>(), false, {receiver} );
      }
   }
   auto id = trx.id();

   auto delay = fc::seconds(trx.delay_sec);
   EOS_ASSERT( delay >= required_delay, transaction_exception,
               "authorization imposes a delay (${required_delay} sec) greater than the delay specified in transaction header (${specified_delay} sec)",
               ("required_delay", required_delay.to_seconds())("specified_delay", delay.to_seconds()) );

   uint32_t trx_size = 0;

   auto& d = control.db();
   d.create<generated_transaction_object>( [&]( auto& gtx ) {
      gtx.trx_id      = id;
      gtx.sender      = receiver;
      gtx.sender_id   = sender_id;
      gtx.payer       = payer;
      gtx.published   = control.pending_block_time();
      gtx.delay_until = gtx.published + delay;
      gtx.expiration  = gtx.delay_until + fc::milliseconds(config::deferred_trx_expiration_window_ms);

      trx_size = gtx.set( trx );
   });

   auto& rl = control.get_mutable_resource_limits_manager();
   rl.add_pending_account_ram_usage( payer, config::billable_size_v<generated_transaction_object> + trx_size );
   checktime( trx_size * 4 ); /// 4 instructions per byte of packed generated trx (estimated)

#if 0
   try {
      trx.set_reference_block(controller.head_block_id()); // No TaPoS check necessary
      trx.sender = receiver;
      controller.validate_transaction_without_state(trx);
      // transaction_api::send_deferred guarantees that trx.execute_after is at least head block time, so no need to check expiration.
      // Any other called of this function needs to similarly meet that precondition.

      controller.validate_expiration_not_too_far(trx, trx.execute_after);
      controller.validate_referenced_accounts(trx);

      controller.validate_uniqueness(trx); // TODO: Move this out of here when we have concurrent shards to somewhere we can check for conflicts between shards.

      const auto& gpo = controller.get_global_properties();
      FC_ASSERT( results.deferred_transactions_count < gpo.configuration.max_generated_transaction_count );

      fc::microseconds delay;

      // privileged accounts can do anything, no need to check auth
      if( !privileged ) {
         // check to make sure the payer has authorized this deferred transaction's storage in RAM
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
            delay = controller..get_authorization_manager().check_authorization(trx.actions, flat_set<public_key_type>(), false, {receiver});
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
#endif
}

void apply_context::cancel_deferred_transaction( const uint128_t& sender_id, account_name sender ) {
   auto& generated_transaction_idx = db.get_mutable_index<generated_transaction_multi_index>();
   const auto* gto = db.find<generated_transaction_object,by_sender_id>(boost::make_tuple(sender, sender_id));
   EOS_ASSERT( gto != nullptr, transaction_exception,
               "there is no generated transaction created by account ${sender} with sender id ${sender_id}",
               ("sender", sender)("sender_id", sender_id) );

   control.get_mutable_resource_limits_manager().add_pending_account_ram_usage(gto->payer, -( config::billable_size_v<generated_transaction_object> + gto->packed_trx.size()));
   generated_transaction_idx.remove(*gto);
   checktime( 100 );
}

const table_id_object* apply_context::find_table( name code, name scope, name table ) {
   return db.find<table_id_object, by_code_scope_table>(boost::make_tuple(code, scope, table));
}

const table_id_object& apply_context::find_or_create_table( name code, name scope, name table, const account_name &payer ) {
   const auto* existing_tid =  db.find<table_id_object, by_code_scope_table>(boost::make_tuple(code, scope, table));
   if (existing_tid != nullptr) {
      return *existing_tid;
   }

   update_db_usage(payer, config::billable_size_v<table_id_object>);

   return db.create<table_id_object>([&](table_id_object &t_id){
      t_id.code = code;
      t_id.scope = scope;
      t_id.table = table;
      t_id.payer = payer;
   });
}

void apply_context::remove_table( const table_id_object& tid ) {
   update_db_usage(tid.payer, - config::billable_size_v<table_id_object>);
   db.remove(tid);
}

vector<account_name> apply_context::get_active_producers() const {
   const auto& ap = control.active_producers();
   vector<account_name> accounts; accounts.reserve( ap.producers.size() );

   for(const auto& producer : ap.producers )
      accounts.push_back(producer.producer_name);

   return accounts;
}

void apply_context::checktime(uint32_t instruction_count) {
   if( BOOST_UNLIKELY(fc::time_point::now() > processing_deadline) ) {
      throw checktime_exceeded();
   }
   cpu_usage += instruction_count;
   FC_ASSERT( cpu_usage <= max_cpu, "contract consumed more cpu cycles than allowed" );
}


bytes apply_context::get_packed_transaction() {
   auto r = fc::raw::pack( static_cast<const transaction&>(trx) );
   checktime( r.size() );
   return r;
}

void apply_context::update_db_usage( const account_name& payer, int64_t delta ) {
   if( (delta > 0) ) {
      if( !(privileged || payer == account_name(receiver)) ) {
         require_authorization( payer );
      }
      mutable_controller.get_mutable_resource_limits_manager().add_pending_account_ram_usage(payer, delta);
   }
}


int apply_context::get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const
{
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

   auto ps = fc::raw::pack_size( *act );
   if( ps <= buffer_size ) {
      fc::datastream<char*> ds(buffer, buffer_size);
      fc::raw::pack( ds, *act );
   }
   return ps;
}

int apply_context::get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const {
   if( index >= trx.context_free_data.size() ) return -1;

   auto s = trx.context_free_data[index].size();

   if( buffer_size == 0 ) return s;

   if( buffer_size < s )
      memcpy( buffer, trx.context_free_data[index].data(), buffer_size );
   else
      memcpy( buffer, trx.context_free_data[index].data(), s );

   return s;
}

void apply_context::check_auth( const transaction& trx, const vector<permission_level>& perm ) {
   control.get_authorization_manager().check_authorization( trx.actions,
                                                            {},
                                                            true,
                                                            {},
                                                            flat_set<permission_level>(perm.begin(), perm.end()) );
}

int apply_context::db_store_i64( uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size ) {
   return db_store_i64( receiver, scope, table, payer, id, buffer, buffer_size);
}

int apply_context::db_store_i64( uint64_t code, uint64_t scope, uint64_t table, const account_name& payer, uint64_t id, const char* buffer, size_t buffer_size ) {
//   require_write_lock( scope );
   const auto& tab = find_or_create_table( code, scope, table, payer );
   auto tableid = tab.id;

   FC_ASSERT( payer != account_name(), "must specify a valid account to pay for new record" );

   const auto& obj = db.create<key_value_object>( [&]( auto& o ) {
      o.t_id        = tableid;
      o.primary_key = id;
      o.value.resize( buffer_size );
      o.payer       = payer;
      memcpy( o.value.data(), buffer, buffer_size );
   });

   db.modify( tab, [&]( auto& t ) {
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

//   require_write_lock( table_obj.scope );

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

   db.modify( obj, [&]( auto& o ) {
     o.value.resize( buffer_size );
     memcpy( o.value.data(), buffer, buffer_size );
     o.payer = payer;
   });
}

void apply_context::db_remove_i64( int iterator ) {
   const key_value_object& obj = keyval_cache.get( iterator );

   const auto& table_obj = keyval_cache.get_table( obj.t_id );
   FC_ASSERT( table_obj.code == receiver, "db access violation" );

//   require_write_lock( table_obj.scope );

   update_db_usage( obj.payer,  -(obj.value.size() + config::billable_size_v<key_value_object>) );

   db.modify( table_obj, [&]( auto& t ) {
      --t.count;
   });
   db.remove( obj );

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
   const auto& idx = db.get_index<key_value_index, by_scope_primary>();

   auto itr = idx.iterator_to( obj );
   ++itr;

   if( itr == idx.end() || itr->t_id != obj.t_id ) return keyval_cache.get_end_iterator_by_table_id(obj.t_id);

   primary = itr->primary_key;
   return keyval_cache.add( *itr );
}

int apply_context::db_previous_i64( int iterator, uint64_t& primary ) {
   const auto& idx = db.get_index<key_value_index, by_scope_primary>();

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
   //require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const key_value_object* obj = db.find<key_value_object, by_scope_primary>( boost::make_tuple( tab->id, id ) );
   if( !obj ) return table_end_itr;

   return keyval_cache.add( *obj );
}

int apply_context::db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   //require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const auto& idx = db.get_index<key_value_index, by_scope_primary>();
   auto itr = idx.lower_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return table_end_itr;
   if( itr->t_id != tab->id ) return table_end_itr;

   return keyval_cache.add( *itr );
}

int apply_context::db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   //require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   auto table_end_itr = keyval_cache.cache_table( *tab );

   const auto& idx = db.get_index<key_value_index, by_scope_primary>();
   auto itr = idx.upper_bound( boost::make_tuple( tab->id, id ) );
   if( itr == idx.end() ) return table_end_itr;
   if( itr->t_id != tab->id ) return table_end_itr;

   return keyval_cache.add( *itr );
}

int apply_context::db_end_i64( uint64_t code, uint64_t scope, uint64_t table ) {
   //require_read_lock( code, scope ); // redundant?

   const auto* tab = find_table( code, scope, table );
   if( !tab ) return -1;

   return keyval_cache.cache_table( *tab );
}


uint64_t apply_context::next_global_sequence() {
   const auto& p = control.get_dynamic_global_properties();
   db.modify( p, [&]( auto& dgp ) {
      ++dgp.global_action_sequence;
   });
   return p.global_action_sequence;
}

uint64_t apply_context::next_recv_sequence( account_name receiver ) {
   const auto& rs = db.get<account_sequence_object,by_name>( receiver );
   db.modify( rs, [&]( auto& mrs ) {
      ++mrs.recv_sequence;
   });
   return rs.recv_sequence;
}
uint64_t apply_context::next_auth_sequence( account_name actor ) {
   const auto& rs = db.get<account_sequence_object,by_name>( actor );
   db.modify( rs, [&](auto& mrs ){
      ++mrs.auth_sequence;
   });
   return rs.auth_sequence;
}


} } /// eosio::chain
