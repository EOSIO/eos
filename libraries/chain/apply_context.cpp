#include <algorithm>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
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

static inline void print_debug(account_name receiver, const action_trace& ar) {
   if(fc::logger::get(DEFAULT_LOGGER).is_enabled(fc::log_level::debug)) {
      if (!ar.console.empty()) {
         auto prefix = fc::format_string(
                                         "\n[(${a},${n})->${r}]",
                                         fc::mutable_variant_object()
                                         ("a", ar.act.account)
                                         ("n", ar.act.name)
                                         ("r", receiver));
         dlog(prefix + ": CONSOLE OUTPUT BEGIN =====================\n"
              + ar.console
              + prefix + ": CONSOLE OUTPUT END   =====================" );
      }
   }
}

action_trace apply_context::exec_one()
{
   auto start = fc::time_point::now();
   cpu_usage = 0;
   cpu_usage_limit = trx_context.get_action_cpu_usage_limit( context_free );
   checktime( control.get_global_properties().configuration.base_per_action_cpu_usage );
   try {
      const auto &a = control.get_account(receiver);
      privileged = a.privileged;
      auto native = control.find_apply_handler(receiver, act.account, act.name);
      if (native) {
         (*native)(*this);
      }

      if (a.code.size() > 0 && !(act.name == N(setcode) && act.account == config::system_account_name)) {
         try {
            control.get_wasm_interface().apply(a.code_version, a.code, *this);
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

   cpu_usage = trx_context.add_action_cpu_usage( cpu_usage, context_free );

   action_trace t(r);
   t.trx_id = trx_context.id;
   t.act = act;
   t.cpu_usage = cpu_usage;
   t.total_cpu_usage = cpu_usage;
   t.console = _pending_console_output.str();

   executed.emplace_back( move(r) );
   total_cpu_usage += cpu_usage;

   print_debug(receiver, t);

   reset_console();

   t.elapsed = fc::time_point::now() - start;
   return t;
}

void apply_context::exec()
{
   _notified.push_back(receiver);
   trace = exec_one();
   for( uint32_t i = 1; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      trace.inline_traces.emplace_back( exec_one() );
      trace.total_cpu_usage += trace.inline_traces.back().total_cpu_usage;
   }

   if( _cfa_inline_actions.size() > 0 || _inline_actions.size() > 0 ) {
      EOS_ASSERT( recurse_depth < control.get_global_properties().configuration.max_inline_action_depth,
                  transaction_exception, "inline action recursion depth reached" );
   }

   for( const auto& inline_action : _cfa_inline_actions ) {
      apply_context ncontext( control, trx_context, inline_action, recurse_depth + 1 );
      ncontext.context_free = true;
      ncontext.exec();
      fc::move_append( executed, move(ncontext.executed) );
      total_cpu_usage += ncontext.trace.total_cpu_usage;
      trace.total_cpu_usage += ncontext.trace.total_cpu_usage;
      trace.inline_traces.emplace_back(ncontext.trace);
   }

   for( const auto& inline_action : _inline_actions ) {
      apply_context ncontext( control, trx_context, inline_action, recurse_depth + 1 );
      ncontext.exec();
      fc::move_append( executed, move(ncontext.executed) );
      total_cpu_usage += ncontext.total_cpu_usage;
      trace.total_cpu_usage += ncontext.trace.total_cpu_usage;
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
   EOS_ASSERT( false, missing_auth_exception, "missing authority of ${account}", ("account",account));
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
  EOS_ASSERT( false, missing_auth_exception, "missing authority of ${account}/${permission}",
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
      if( a.account != receiver ) { // if a contract is calling itself then there is no need to check permissions
         const auto delay = control.limit_delay( control.get_authorization_manager()
                                                        .check_authorization( {a},
                                                                              flat_set<public_key_type>(),
                                                                              false,
                                                                              {receiver}                   ) );
         FC_ASSERT( trx_context.published + delay <= control.pending_block_time(),
                    "authorization for inline action imposes a delay of ${delay} seconds that is not met",
                    ("delay", delay.to_seconds()) );

         //QUESTION: Is it smart to allow a deferred transaction that has been delayed for some time to get away
         //          with sending an inline action that requires a delay even though the decision to send that inline
         //          action was made at the moment the deferred transaction was executed with potentially no forewarning?
      }
   }
   _inline_actions.emplace_back( move(a) );
}

void apply_context::execute_context_free_inline( action&& a ) {
   FC_ASSERT( a.authorization.size() == 0, "context free actions cannot have authorizations" );
   _cfa_inline_actions.emplace_back( move(a) );
}


void apply_context::schedule_deferred_transaction( const uint128_t& sender_id, account_name payer, transaction&& trx ) {
   trx.set_reference_block(control.head_block_id()); // No TaPoS check necessary
   //trx.validate(); // Not needed anymore since overflow is prevented by using uint64_t instead of uint32_t
   FC_ASSERT( trx.context_free_actions.size() == 0, "context free actions are not currently allowed in generated transactions" );
   control.validate_referenced_accounts( trx );
   control.validate_expiration( trx );

   // Charge ahead of time for the additional net usage needed to retire the deferred transaction
   // whether that be by successfully executing, soft failure, hard failure, or expiration.
   const auto& cfg = control.get_global_properties().configuration;
   trx_context.add_net_usage( static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                               + static_cast<uint64_t>(config::transaction_id_net_usage) ); // Will exit early if net usage cannot be payed.

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
         required_delay = control.limit_delay( control.get_authorization_manager()
                                                      .check_authorization( trx.actions,
                                                                            flat_set<public_key_type>(),
                                                                            false,
                                                                            {receiver}                   ) );

      }
   }

   auto delay = fc::seconds(trx.delay_sec);
   EOS_ASSERT( delay >= required_delay, transaction_exception,
               "authorization imposes a delay (${required_delay} sec) greater than the delay specified in transaction header (${specified_delay} sec)",
               ("required_delay", required_delay.to_seconds())("specified_delay", delay.to_seconds()) );


   uint32_t trx_size = 0;
   auto& d = control.db();
   if ( auto ptr = d.find<generated_transaction_object,by_sender_id>(boost::make_tuple(receiver, sender_id)) ) {
      d.modify<generated_transaction_object>( *ptr, [&]( auto& gtx ) {
            gtx.sender      = receiver;
            gtx.sender_id   = sender_id;
            gtx.payer       = payer;
            gtx.published   = control.pending_block_time();
            gtx.delay_until = gtx.published + delay;
            gtx.expiration  = gtx.delay_until + fc::seconds(control.get_global_properties().configuration.deferred_trx_expiration_window);

            trx_size = gtx.set( trx );
         });
   } else {
      d.create<generated_transaction_object>( [&]( auto& gtx ) {
            gtx.trx_id      = trx_context.id;
            gtx.sender      = receiver;
            gtx.sender_id   = sender_id;
            gtx.payer       = payer;
            gtx.published   = control.pending_block_time();
            gtx.delay_until = gtx.published + delay;
            gtx.expiration  = gtx.delay_until + fc::seconds(control.get_global_properties().configuration.deferred_trx_expiration_window);

            trx_size = gtx.set( trx );
         });
   }

   trx_context.add_ram_usage( payer, (config::billable_size_v<generated_transaction_object> + trx_size) );
   checktime( trx_size * 4 ); /// 4 instructions per byte of packed generated trx (estimated)
}

void apply_context::cancel_deferred_transaction( const uint128_t& sender_id, account_name sender ) {
   auto& generated_transaction_idx = db.get_mutable_index<generated_transaction_multi_index>();
   const auto* gto = db.find<generated_transaction_object,by_sender_id>(boost::make_tuple(sender, sender_id));
   EOS_ASSERT( gto != nullptr, transaction_exception,
               "there is no generated transaction created by account ${sender} with sender id ${sender_id}",
               ("sender", sender)("sender_id", sender_id) );

   trx_context.add_ram_usage( gto->payer, -(config::billable_size_v<generated_transaction_object> + gto->packed_trx.size()) );
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

void apply_context::reset_console() {
   _pending_console_output = std::ostringstream();
   _pending_console_output.setf( std::ios::scientific, std::ios::floatfield );
}

void apply_context::checktime(uint32_t instruction_count) {
   cpu_usage += instruction_count;
   EOS_ASSERT( BOOST_LIKELY(cpu_usage <= cpu_usage_limit), tx_cpu_usage_exceeded, "action cpu usage exceeded" );
   trx_context.check_time();
}

bytes apply_context::get_packed_transaction() {
   auto r = fc::raw::pack( static_cast<const transaction&>(trx_context.trx) );
   checktime( r.size() );
   return r;
}

void apply_context::update_db_usage( const account_name& payer, int64_t delta ) {
   if( (delta > 0) ) {
      if( !(privileged || payer == account_name(receiver)) ) {
         require_authorization( payer );
      }
      trx_context.add_ram_usage(payer, delta);
   }
}


int apply_context::get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const
{
   const auto& trx = trx_context.trx;
   const action* act_ptr = nullptr;

   if( type == 0 ) {
      if( index >= trx.context_free_actions.size() )
         return -1;
      act_ptr = &trx.context_free_actions[index];
   }
   else if( type == 1 ) {
      if( index >= trx.actions.size() )
         return -1;
      act_ptr = &trx.actions[index];
   }

   auto ps = fc::raw::pack_size( *act_ptr );
   if( ps <= buffer_size ) {
      fc::datastream<char*> ds(buffer, buffer_size);
      fc::raw::pack( ds, *act_ptr );
   }
   return ps;
}

int apply_context::get_context_free_data( uint32_t index, char* buffer, size_t buffer_size )const
{
   const auto& trx = trx_context.trx;

   if( index >= trx.context_free_data.size() ) return -1;

   auto s = trx.context_free_data[index].size();
   if( buffer_size == 0 ) return s;

   auto copy_size = std::min( buffer_size, s );
   memcpy( buffer, trx.context_free_data[index].data(), copy_size );

   return copy_size;
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

   auto s = obj.value.size();
   if( buffer_size == 0 ) return s;

   auto copy_size = std::min( buffer_size, s );
   memcpy( buffer, obj.value.data(), copy_size );

   return copy_size;
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
