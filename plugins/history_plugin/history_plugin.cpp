#include <eosio/history_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <fc/io/json.hpp>

namespace eosio { 
   using namespace chain;

   static appbase::abstract_plugin& _history_plugin = app().register_plugin<history_plugin>();


   struct account_history_object : public chainbase::object<account_history_object_type, account_history_object>  {
      OBJECT_CTOR( account_history_object );

      id_type      id;
      account_name account; ///< the name of the account which has this action in its history
      uint64_t     action_sequence_num = 0; ///< the sequence number of the relevant action (global)
      int32_t      account_sequence_num = 0; ///< the sequence number for this account (per-account)
   };

   struct action_history_object : public chainbase::object<action_history_object_type, action_history_object> {

      OBJECT_CTOR( action_history_object, (packed_action_trace) );

      id_type      id;
      uint64_t     action_sequence_num; ///< the sequence number of the relevant action

      shared_string        packed_action_trace;
      uint32_t             block_num;
      block_timestamp_type block_time;
      transaction_id_type  trx_id;
   };
   using account_history_id_type = account_history_object::id_type;
   using action_history_id_type  = action_history_object::id_type;


   struct by_action_sequence_num;
   struct by_account_action_seq;
   struct by_trx_id;

   using action_history_index = chainbase::shared_multi_index_container<
      action_history_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<action_history_object, action_history_object::id_type, &action_history_object::id>>,
         ordered_unique<tag<by_action_sequence_num>, member<action_history_object, uint64_t, &action_history_object::action_sequence_num>>,
         ordered_unique<tag<by_trx_id>, 
            composite_key< action_history_object,
               member<action_history_object, transaction_id_type, &action_history_object::trx_id>,
               member<action_history_object, uint64_t, &action_history_object::action_sequence_num >
            >
         >
      >
   >;

   using account_history_index = chainbase::shared_multi_index_container<
      account_history_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_history_object, account_history_object::id_type, &account_history_object::id>>,
         ordered_unique<tag<by_account_action_seq>, 
            composite_key< account_history_object,
               member<account_history_object, account_name, &account_history_object::account >,
               member<account_history_object, int32_t, &account_history_object::account_sequence_num >
            >
         >
      >
   >;

} /// namespace eosio

CHAINBASE_SET_INDEX_TYPE(eosio::account_history_object, eosio::account_history_index)
CHAINBASE_SET_INDEX_TYPE(eosio::action_history_object, eosio::action_history_index)

namespace eosio {

   class history_plugin_impl {
      public:
         std::set<account_name> filter_on;
         chain_plugin*          chain_plug = nullptr;

         bool is_filtered( const account_name& n ) {
            return !filter_on.size() || filter_on.find(n) != filter_on.end();
         }
         bool filter( const action_trace& act ) {
            if( filter_on.size() == 0 ) return true;
            if( is_filtered( act.receipt.receiver ) ) return true;
            for( const auto& a : act.act.authorization ) {
               if( is_filtered(  a.actor ) ) return true;
            }
            return false;
         }

         set<account_name> account_set( const action_trace& act ) {
            set<account_name> result;

            if( is_filtered( act.receipt.receiver ) )
               result.insert( act.receipt.receiver );
            for( const auto& a : act.act.authorization ) {
               if( is_filtered(  a.actor ) ) result.insert( a.actor );
            }
            return result;
         }

         void record_account_action( account_name n, const base_action_trace& act ) {
            auto& chain = chain_plug->chain();
            auto& db = chain.db();

            const auto& idx = db.get_index<account_history_index, by_account_action_seq>();
            auto itr = idx.lower_bound( boost::make_tuple( name(n.value+1), 0 ) );

            uint64_t asn = 0;
            if( itr != idx.begin() ) --itr;
            if( itr->account == n ) 
               asn = itr->account_sequence_num + 1;

            //idump((n)(act.receipt.global_sequence)(asn));
            const auto& a = db.create<account_history_object>( [&]( auto& aho ) {
              aho.account = n;
              aho.action_sequence_num = act.receipt.global_sequence;
              aho.account_sequence_num = asn;
            });
            //idump((a.account)(a.action_sequence_num)(a.action_sequence_num));
         }

         void on_action_trace( const action_trace& at ) {
            if( filter( at ) ) {
               //idump((fc::json::to_pretty_string(at)));
               auto& chain = chain_plug->chain();
               auto& db = chain.db();

               db.create<action_history_object>( [&]( auto& aho ) {
                  auto ps = fc::raw::pack_size( at );
                  aho.packed_action_trace.resize(ps);
                  datastream<char*> ds( aho.packed_action_trace.data(), ps );
                  fc::raw::pack( ds, at );
                  aho.action_sequence_num = at.receipt.global_sequence;
                  aho.block_num = chain.pending_block_state()->block_num;
                  aho.block_time = chain.pending_block_time();
                  aho.trx_id     = at.trx_id;
               });
               
               auto aset = account_set( at );
               for( auto a : aset ) {
                  record_account_action( a, at );
               }
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
      auto& chain = my->chain_plug->chain();

      chain.db().add_index<account_history_index>();
      chain.db().add_index<action_history_index>();

      chain.applied_transaction.connect( [&]( const transaction_trace_ptr& p ){
            my->on_applied_transaction(p);
      });

   }

   void history_plugin::plugin_startup() {
   }

   void history_plugin::plugin_shutdown() {
   }




   namespace history_apis { 
      read_only::get_actions_result read_only::get_actions( const read_only::get_actions_params& params )const {
         edump((params));
        auto& chain = history->chain_plug->chain();
        const auto& db = chain.db();

        const auto& idx = db.get_index<account_history_index, by_account_action_seq>();

        int32_t start = 0;
        int32_t pos = params.pos ? *params.pos : -1;
        int32_t end = 0;
        int32_t offset = params.offset ? *params.offset : -20;
        auto n = params.account_name;
        idump((pos));
        if( pos == -1 ) {
            auto itr = idx.lower_bound( boost::make_tuple( name(n.value+1), 0 ) );
            if( itr == idx.begin() ) {
               if( itr->account == n )
                  pos = itr->account_sequence_num+1;
            } else if( itr != idx.begin() ) --itr;

            if( itr->account == n ) 
               pos = itr->account_sequence_num + 1;
        }

        if( pos== -1 ) pos = 0xfffffff;

        if( offset > 0 ) {
           start = pos;
           end   = start + offset;
        } else {
           start = pos + offset;
           if( start > pos ) start = 0;
           end   = pos;
        }
        FC_ASSERT( end >= start );

        idump((start)(end));

        auto start_itr = idx.lower_bound( boost::make_tuple( n, start ) );
        auto end_itr = idx.lower_bound( boost::make_tuple( n, end+1) );

        auto start_time = fc::time_point::now();
        auto end_time = start_time;

        get_actions_result result;
        result.last_irreversible_block = chain.last_irreversible_block_num();
        while( start_itr != end_itr ) {
           const auto& a = db.get<action_history_object, by_action_sequence_num>( start_itr->action_sequence_num );
           fc::datastream<const char*> ds( a.packed_action_trace.data(), a.packed_action_trace.size() );
           action_trace t;
           fc::raw::unpack( ds, t );
           result.actions.emplace_back( ordered_action_result{
                                 start_itr->action_sequence_num,
                                 start_itr->account_sequence_num,
                                 a.block_num, a.block_time,
                                 chain.to_variant_with_abi(t)
                                 });

           end_time = fc::time_point::now();
           if( end_time - start_time > fc::microseconds(100000) ) {
              result.time_limit_exceeded_error = true;
              break;
           }
           ++start_itr;
        }
        return result;
      }


      read_only::get_transaction_result read_only::get_transaction( const read_only::get_transaction_params& p )const {
         auto& chain = history->chain_plug->chain();

         get_transaction_result result;

         result.id = p.id;
         result.last_irreversible_block = chain.last_irreversible_block_num();

         const auto& db = chain.db();

         const auto& idx = db.get_index<action_history_index, by_trx_id>();
         auto itr = idx.lower_bound( boost::make_tuple(p.id) );
         if( itr == idx.end() ) {
            return result;
         }
         result.id         = itr->trx_id;
         result.block_num  = itr->block_num;
         result.block_time = itr->block_time;

         if( fc::variant(result.id).as_string().substr(0,8) != fc::variant(p.id).as_string().substr(0,8) )
            return result;

         while( itr != idx.end() && itr->trx_id == result.id ) {

           fc::datastream<const char*> ds( itr->packed_action_trace.data(), itr->packed_action_trace.size() );
           action_trace t;
           fc::raw::unpack( ds, t );
           result.traces.emplace_back( chain.to_variant_with_abi(t) );

           ++itr;
         }

         auto blk = chain.fetch_block_by_number( result.block_num );
         if( blk == nullptr ) { // still in pending
             auto blk_state = chain.pending_block_state();
             if( blk_state != nullptr ) {
                 blk = blk_state->block;
             }
         }
         if( blk != nullptr ) {
             for (const auto &receipt: blk->transactions) {
                 if (receipt.trx.contains<packed_transaction>()) {
                     auto &pt = receipt.trx.get<packed_transaction>();
                     auto mtrx = transaction_metadata(pt);
                     if (mtrx.id == result.id) {
                         fc::mutable_variant_object r("receipt", receipt);
                         r("trx", chain.to_variant_with_abi(mtrx.trx));
                         result.trx = move(r);
                         break;
                     }
                 } else {
                     auto &id = receipt.trx.get<transaction_id_type>();
                     if (id == result.id) {
                         fc::mutable_variant_object r("receipt", receipt);
                         result.trx = move(r);
                         break;
                     }
                 }
             }
         }

         return result;
      }

   } /// history_apis



} /// namespace eosio
