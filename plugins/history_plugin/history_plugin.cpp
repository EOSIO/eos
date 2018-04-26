#include <eosio/history_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <fc/io/json.hpp>

namespace eosio { 
   using namespace chain;

   class history_plugin_impl {
      public:
         std::set<account_name> filter_on;
         chain_plugin*          chain_plug = nullptr;

         bool is_filtered( const account_name& n ) {
            return filter_on.find(n) != filter_on.end();
         }
         bool filter( const action_trace& act ) {
            if( filter_on.size() == 0 ) return true;
            if( is_filtered( act.receipt.receiver ) ) return true;
            for( const auto& a : act.act.authorization ) {
               if( is_filtered(  a.actor ) ) return true;
            }
            return false;
         }

         void on_action_trace( const action_trace& at ) {
            if( filter( at ) ) {
               idump((fc::json::to_pretty_string(at.act)));
            }
            for( const auto& iline : at.inline_traces ) {
               on_action_trace( iline );
            }
         }

         void on_applied_transaction( const transaction_trace_ptr& trace ) {
            for( const auto& atrace : trace->action_traces ) {
               on_action_trace( atrace );
            }
         }
   };

   history_plugin::history_plugin()
   :my(std::make_shared<history_plugin_impl>()) {
   }

   history_plugin::~history_plugin() {
   }


   void history_plugin::set_program_options(options_description& cli, options_description& cfg) {
      cfg.add_options()
            ("filter_on_accounts,f", bpo::value<vector<string>>()->composing(),
             "Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.")
            ;
   }

   void history_plugin::plugin_initialize(const variables_map& options) {
      if(options.count("filter_on_accounts"))
      {
         auto foa = options.at("filter_on_accounts").as<vector<string>>();
         for(auto filter_account : foa)
            my->filter_on.emplace(filter_account);
      }

      my->chain_plug = app().find_plugin<chain_plugin>();
      my->chain_plug->chain().applied_transaction.connect( [&]( const transaction_trace_ptr& p ){
            my->on_applied_transaction(p);
      });

   }

   void history_plugin::plugin_startup() {
   }

   void history_plugin::plugin_shutdown() {
   }


} /// namespace eosio
