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

         if (a.code.size() > 0) {
            // get code from cache
            auto code = mutable_controller.get_wasm_cache().checkout_scoped(a.code_version, a.code.data(),
                                                                            a.code.size());

            // get wasm_interface
            auto &wasm = wasm_interface::get();
            wasm.apply(code, *this);
         }
      }

      if (results.read_scopes.empty()) {
         std::sort(results.read_scopes.begin(), results.read_scopes.end());
      }
      if (results.write_scopes.empty()) {
         std::sort(results.write_scopes.begin(), results.write_scopes.end());
      }
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

   // create a receipt for this
   vector<data_access_info> data_access;
   data_access.reserve(results.write_scopes.size() + results.read_scopes.size());
   for (const auto& scope: results.write_scopes) {
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

   for (const auto& scope: results.read_scopes) {
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

static bool scopes_contain(const vector<scope_name>& scopes, const scope_name& scope) {
   for (const auto &s : scopes) {
      if (s == scope) {
         return true;
      }
   }

   return false;
}

void apply_context::require_write_scope(const scope_name& scope) {
   if (trx_meta.allowed_write_scopes) {
      EOS_ASSERT( scopes_contain(**trx_meta.allowed_write_scopes, scope), tx_missing_write_scope, "missing write scope ${scope}", ("scope",scope) );
   }

   if (!scopes_contain(results.write_scopes, scope)) {
      results.write_scopes.emplace_back(scope);
   }
}

void apply_context::require_read_scope(const scope_name& scope) {
   if (trx_meta.allowed_read_scopes) {
      EOS_ASSERT( scopes_contain(**trx_meta.allowed_read_scopes, scope), tx_missing_read_scope, "missing read scope ${scope}", ("scope",scope) );
   }

   if (!scopes_contain(results.read_scopes, scope)) {
      results.read_scopes.emplace_back(scope);
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

const contracts::table_id_object* apply_context::find_table( name scope, name code, name table ) {
   require_read_scope(scope);
   return db.find<table_id_object, contracts::by_scope_code_table>(boost::make_tuple(scope, code, table));
}

const contracts::table_id_object& apply_context::find_or_create_table( name scope, name code, name table ) {
   require_read_scope(scope);
   const auto* existing_tid =  db.find<contracts::table_id_object, contracts::by_scope_code_table>(boost::make_tuple(scope, code, table));
   if (existing_tid != nullptr) {
      return *existing_tid;
   }

   require_write_scope(scope);
   return mutable_db.create<contracts::table_id_object>([&](contracts::table_id_object &t_id){
      t_id.scope = scope;
      t_id.code = code;
      t_id.table = table;
   });
}

} } /// eosio::chain
