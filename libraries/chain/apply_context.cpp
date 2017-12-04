#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/scope_sequence_object.hpp>

namespace eosio { namespace chain {
void apply_context::exec_one()
{
   auto native = mutable_controller.find_apply_handler( receiver, act.scope, act.name );
   if( native ) {
      (*native)(*this);
   } else {
      const auto& a = mutable_controller.get_database().get<account_object,by_name>(receiver);

      if (a.code.size() > 0) {
         // get code from cache
         auto code = mutable_controller.get_wasm_cache().checkout_scoped(a.code_version, a.code.data(), a.code.size());

         // get wasm_interface
         auto& wasm = wasm_interface::get();
         wasm.apply(code, *this);
      }
   }

   // create a receipt for this
   vector<data_access_info> data_access;
   data_access.reserve(trx.write_scope.size() + trx.read_scope.size());
   for (const auto& scope: trx.write_scope) {
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
         data_access.emplace_back(data_access_info{data_access_info::write, scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::write, scope, scope_sequence->sequence});
         try {
            mutable_controller.get_mutable_database().modify(*scope_sequence, [&](scope_sequence_object& ss) {
               ss.sequence += 1;
            });
         } FC_CAPTURE_AND_RETHROW((scope)(receiver));
      }
   }

   for (const auto& scope: trx.read_scope) {
      auto key = boost::make_tuple(scope, receiver);
      const auto& scope_sequence = mutable_controller.get_database().find<scope_sequence_object, by_scope_receiver>(key);
      if (scope_sequence == nullptr) {
         data_access.emplace_back(data_access_info{data_access_info::read, scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::read, scope, scope_sequence->sequence});
      }
   }

   results.applied_actions.emplace_back(action_trace {receiver, act, _pending_console_output.str(), 0, 0, move(data_access)});
   _pending_console_output = std::ostringstream();
}

void apply_context::exec()
{
   exec_one();

   for( uint32_t i = 0; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      exec_one();
   }

   for( uint32_t i = 0; i < _inline_actions.size(); ++i ) {
      apply_context ncontext( mutable_controller, mutable_db, trx, _inline_actions[i], _inline_actions[i].scope );
      ncontext.exec();
      append_results(move(ncontext.results));
   }

} /// exec()

void apply_context::require_authorization( const account_name& account )const {
  for( const auto& auth : act.authorization )
     if( auth.actor == account ) return;
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

void apply_context::require_write_scope(const account_name& account)const {
   for( const auto& s : trx.write_scope )
      if( s == account ) return;

   if( trx.write_scope.size() == 1 && trx.write_scope.front() == config::eosio_all_scope )
      return;

   EOS_ASSERT( false, tx_missing_write_scope, "missing write scope ${account}", 
               ("account",account) );
}

void apply_context::require_read_scope(const account_name& account)const {
   for( const auto& s : trx.write_scope )
      if( s == account ) return;
   for( const auto& s : trx.read_scope )
      if( s == account ) return;

   if( trx.write_scope.size() == 1 && trx.write_scope.front() == config::eosio_all_scope )
      return;

   EOS_ASSERT( false, tx_missing_read_scope, "missing read scope ${account}", 
               ("account",account) );
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

void apply_context::deferred_transaction_start( uint32_t id, 
                                 uint16_t region,
                                 vector<scope_name> write_scopes, 
                                 vector<scope_name> read_scopes,
                                 time_point_sec     execute_after,
                                 time_point_sec     execute_before
                               ) {
   FC_ASSERT( execute_before > (controller.head_block_time() + fc::milliseconds(2*config::block_interval_ms)),
                                "transaction is expired when created" );
   FC_ASSERT( execute_after < execute_before );

   /// TODO: make default_max_gen_trx_count a producer parameter
   FC_ASSERT( _pending_deferred_transactions.size() < config::default_max_gen_trx_count );

   auto itr = _pending_deferred_transactions.find( id );
   FC_ASSERT( itr == _pending_deferred_transactions.end(), "pending transaction with ID ${id} already started", ("id",id) );
   auto& trx = _pending_deferred_transactions[id];
   trx.region        = region;
   trx.write_scope   = move( write_scopes );
   trx.read_scope    = move( read_scopes );
   trx.expiration    = execute_before;
   trx.execute_after = execute_after;
   trx.sender        = receiver; ///< sender is the receiver of the current action
   trx.sender_id     = id;

   controller.validate_scope( trx );

   /// TODO: make sure there isn't already a deferred transaction with this ID
}

deferred_transaction& apply_context::get_deferred_transaction( uint32_t id ) {
   auto itr = _pending_deferred_transactions.find( id );
   FC_ASSERT( itr != _pending_deferred_transactions.end(), "attempt to reference unknown pending deferred transaction" );
   return itr->second;
}

void apply_context::deferred_transaction_append( uint32_t id, action a ) {
//   auto& dt = get_deferred_transaction(id);
//   dt.actions.emplace_back( move(a) );
//
//   /// TODO: use global properties object for dynamic configuration of this default_max_gen_trx_size
//   FC_ASSERT( fc::raw::pack_size( dt ) < config::default_max_gen_trx_size, "generated transaction too big" );
}
void apply_context::deferred_transaction_send( uint32_t id ) {
//   auto& dt = get_deferred_transaction(id);
//   FC_ASSERT( dt.actions.size(), "transaction must contain at least one action" );
//   controller.check_authorization( dt, flat_set<public_key_type>(), false, {receiver} );
//   auto itr = _pending_deferred_transactions.find( id );
//   _pending_deferred_transactions.erase(itr);
}

} } /// eosio::chain
