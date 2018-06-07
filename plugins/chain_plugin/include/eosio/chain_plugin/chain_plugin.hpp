/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/plugin_interface.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fc/static_variant.hpp>

namespace fc { class variant; }

namespace eosio {
   using chain::controller;
   using std::unique_ptr;
   using namespace appbase;
   using chain::name;
   using chain::uint128_t;
   using chain::public_key_type;
   using fc::optional;
   using boost::container::flat_set;
   using chain::asset;
   using chain::authority;
   using chain::account_name;
   using chain::abi_def;
   using chain::abi_serializer;

namespace chain_apis {
struct empty{};

struct permission {
   name              perm_name;
   name              parent;
   authority         required_auth;
};

template<typename>
struct resolver_factory;

class read_only {
   const controller& db;

public:
   static const string KEYi64;

   read_only(const controller& db)
      : db(db) {}

   using get_info_params = empty;

   struct get_info_results {
      string                  server_version;
      chain::chain_id_type    chain_id;
      uint32_t                head_block_num = 0;
      uint32_t                last_irreversible_block_num = 0;
      chain::block_id_type    last_irreversible_block_id;
      chain::block_id_type    head_block_id;
      fc::time_point_sec      head_block_time;
      account_name            head_block_producer;

      uint64_t                virtual_block_cpu_limit = 0;
      uint64_t                virtual_block_net_limit = 0;

      uint64_t                block_cpu_limit = 0;
      uint64_t                block_net_limit = 0;
      //string                  recent_slots;
      //double                  participation_rate = 0;
   };
   get_info_results get_info(const get_info_params&) const;

   struct producer_info {
      name                       producer_name;
   };

   using account_resource_limit = chain::resource_limits::account_resource_limit;

   struct get_account_results {
      name                       account_name;
      bool                       privileged = false;
      fc::time_point             last_code_update;
      fc::time_point             created;

      int64_t                    ram_quota  = 0;
      int64_t                    net_weight = 0;
      int64_t                    cpu_weight = 0;

      account_resource_limit     net_limit;
      account_resource_limit     cpu_limit;
      int64_t                    ram_usage = 0;

      vector<permission>         permissions;

      fc::variant                total_resources;
      fc::variant                self_delegated_bandwidth;
      fc::variant                voter_info;
   };

   struct get_account_params {
      name account_name;
   };
   get_account_results get_account( const get_account_params& params )const;


   struct get_code_results {
      name                   account_name;
      string                 wast;
      string                 wasm;
      fc::sha256             code_hash;
      optional<abi_def>      abi;
   };

   struct get_code_params {
      name account_name;
      bool code_as_wasm = false;
   };

   struct get_abi_results {
      name                   account_name;
      optional<abi_def>      abi;
   };

   struct get_abi_params {
      name account_name;
   };


   get_code_results get_code( const get_code_params& params )const;
   get_abi_results get_abi( const get_abi_params& params )const;



   struct abi_json_to_bin_params {
      name         code;
      name         action;
      fc::variant  args;
   };
   struct abi_json_to_bin_result {
      vector<char>   binargs;
   };

   abi_json_to_bin_result abi_json_to_bin( const abi_json_to_bin_params& params )const;


   struct abi_bin_to_json_params {
      name         code;
      name         action;
      vector<char> binargs;
   };
   struct abi_bin_to_json_result {
      fc::variant    args;
   };

   abi_bin_to_json_result abi_bin_to_json( const abi_bin_to_json_params& params )const;


   struct get_required_keys_params {
      fc::variant transaction;
      flat_set<public_key_type> available_keys;
   };
   struct get_required_keys_result {
      flat_set<public_key_type> required_keys;
   };

   get_required_keys_result get_required_keys( const get_required_keys_params& params)const;


   struct get_block_params {
      string block_num_or_id;
   };

   fc::variant get_block(const get_block_params& params) const;

   struct get_block_header_state_params {
      string block_num_or_id;
   };

   fc::variant get_block_header_state(const get_block_header_state_params& params) const;

   struct get_table_rows_params {
      bool        json = false;
      name        code;
      string      scope;
      name        table;
//      string      table_type;
      string      table_key;
      string      lower_bound;
      string      upper_bound;
      uint32_t    limit = 10;
    };

   struct get_table_rows_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object
      bool                more = false; ///< true if last element in data is not the end and sizeof data() < limit
   };

   get_table_rows_result get_table_rows( const get_table_rows_params& params )const;

   struct get_currency_balance_params {
      name             code;
      name             account;
      optional<string> symbol;
   };

   vector<asset> get_currency_balance( const get_currency_balance_params& params )const;

   struct get_currency_stats_params {
      name           code;
      string         symbol;
   };


   struct get_currency_stats_result {
      asset          supply;
      asset          max_supply;
      account_name   issuer;
   };

   fc::variant get_currency_stats( const get_currency_stats_params& params )const;

   struct get_producers_params {
      bool        json = false;
      string      lower_bound;
      uint32_t    limit = 50;
   };

   struct get_producers_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex string or JSON object
      double              total_producer_vote_weight;
      string              more; ///< fill lower_bound with this value to fetch more rows
   };

   get_producers_result get_producers( const get_producers_params& params )const;

   static void copy_inline_row(const chain::key_value_object& obj, vector<char>& data) {
      data.resize( obj.value.size() );
      memcpy( data.data(), obj.value.data(), obj.value.size() );
   }

   template<typename IndexType, typename Scope, typename Function>
   void walk_table(const name& code, const name& scope, const name& table, Function f) const
   {
      const auto& d = db.db();
      const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(code, scope, table));
      if (t_id != nullptr) {
         const auto &idx = d.get_index<IndexType, Scope>();
         decltype(t_id->id) next_tid(t_id->id._id + 1);
         auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
         auto upper = idx.lower_bound(boost::make_tuple(next_tid));

         for (auto itr = lower; itr != upper; ++itr) {
            if (!f(*itr)) {
               break;
            }
         }
      }
   }

   template <typename IndexType, typename Scope>
   read_only::get_table_rows_result get_table_rows_ex( const read_only::get_table_rows_params& p, const abi_def& abi )const {
      read_only::get_table_rows_result result;
      const auto& d = db.db();

      uint64_t scope = 0;
      try {
         name s(p.scope);
         scope = s.value;
      } catch( ... ) {
         try {
            auto trimmed_scope_str = p.scope;
            boost::trim(trimmed_scope_str);
            scope = boost::lexical_cast<uint64_t>(trimmed_scope_str.c_str(), trimmed_scope_str.size());
         } catch( ... ) {
            try {
               auto symb = eosio::chain::symbol::from_string(p.scope);
               scope = symb.value();
            } catch( ... ) {
               try {
                  scope = ( eosio::chain::string_to_symbol( 0, p.scope.c_str() ) >> 8 );
               } catch( ... ) {
                  FC_ASSERT( false, "could not convert scope string to any of the following: uint64_t, valid name, or valid symbol (with or without the precision)" );
               }
            }
         }
      }

      abi_serializer abis;
      abis.set_abi(abi);
      const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(p.code, scope, p.table));
      if (t_id != nullptr) {
         const auto &idx = d.get_index<IndexType, Scope>();
         decltype(t_id->id) next_tid(t_id->id._id + 1);
         auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
         auto upper = idx.lower_bound(boost::make_tuple(next_tid));

         if (p.lower_bound.size()) {
            lower = idx.lower_bound(boost::make_tuple(t_id->id, fc::variant(
               p.lower_bound).as<typename IndexType::value_type::key_type>()));
         }
         if (p.upper_bound.size()) {
            upper = idx.lower_bound(boost::make_tuple(t_id->id, fc::variant(
               p.upper_bound).as<typename IndexType::value_type::key_type>()));
         }

         vector<char> data;

         auto end = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time

         unsigned int count = 0;
         auto itr = lower;
         for (itr = lower; itr != upper; ++itr) {
            copy_inline_row(*itr, data);

            if (p.json) {
               result.rows.emplace_back(abis.binary_to_variant(abis.get_table_type(p.table), data));
            } else {
               result.rows.emplace_back(fc::variant(data));
            }

            if (++count == p.limit || fc::time_point::now() > end) {
               break;
            }
         }
         if (itr != upper) {
            result.more = true;
         }
      }
      return result;
   }

   friend struct resolver_factory<read_only>;
};

class read_write {
   controller& db;
public:
   read_write(controller& db) : db(db) {}

   using push_block_params = chain::signed_block;
   using push_block_results = empty;
   void push_block(const push_block_params& params, chain::plugin_interface::next_function<push_block_results> next);

   using push_transaction_params = fc::variant_object;
   struct push_transaction_results {
      chain::transaction_id_type  transaction_id;
      fc::variant                 processed;
   };
   void push_transaction(const push_transaction_params& params, chain::plugin_interface::next_function<push_transaction_results> next);


   using push_transactions_params  = vector<push_transaction_params>;
   using push_transactions_results = vector<push_transaction_results>;
   void push_transactions(const push_transactions_params& params, chain::plugin_interface::next_function<push_transactions_results> next);

   friend resolver_factory<read_write>;
};
} // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   chain_apis::read_only get_read_only_api() const { return chain_apis::read_only(chain()); }
   chain_apis::read_write get_read_write_api();

   void accept_block( const chain::signed_block_ptr& block );
   void accept_transaction(const chain::packed_transaction& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

   bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

   bool recover_reversible_blocks( const fc::path& db_dir,
                                   uint32_t cache_size,
                                   optional<fc::path> new_db_dir = optional<fc::path>(),
                                   uint32_t truncate_at_block = 0
                                 )const;

   // Only call this in plugin_initialize() to modify controller constructor configuration
   controller::config& chain_config();
   // Only call this after plugin_startup()!
   controller& chain();
   // Only call this after plugin_startup()!
   const controller& chain() const;

   chain::chain_id_type get_chain_id() const;

private:
   unique_ptr<class chain_plugin_impl> my;
};

}

FC_REFLECT( eosio::chain_apis::permission, (perm_name)(parent)(required_auth) )
FC_REFLECT(eosio::chain_apis::empty, )
FC_REFLECT(eosio::chain_apis::read_only::get_info_results,
(server_version)(chain_id)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)(head_block_id)(head_block_time)(head_block_producer)(virtual_block_cpu_limit)(virtual_block_net_limit)(block_cpu_limit)(block_net_limit) )
FC_REFLECT(eosio::chain_apis::read_only::get_block_params, (block_num_or_id))
FC_REFLECT(eosio::chain_apis::read_only::get_block_header_state_params, (block_num_or_id))

FC_REFLECT( eosio::chain_apis::read_write::push_transaction_results, (transaction_id)(processed) )

FC_REFLECT( eosio::chain_apis::read_only::get_table_rows_params, (json)(code)(scope)(table)(table_key)(lower_bound)(upper_bound)(limit) )
FC_REFLECT( eosio::chain_apis::read_only::get_table_rows_result, (rows)(more) );

FC_REFLECT( eosio::chain_apis::read_only::get_currency_balance_params, (code)(account)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_params, (code)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_result, (supply)(max_supply)(issuer));

FC_REFLECT( eosio::chain_apis::read_only::get_producers_params, (json)(lower_bound)(limit) )
FC_REFLECT( eosio::chain_apis::read_only::get_producers_result, (rows)(total_producer_vote_weight)(more) );

FC_REFLECT( eosio::chain_apis::read_only::get_account_results, (account_name)(privileged)(last_code_update)(created)(ram_quota)(net_weight)(cpu_weight)(net_limit)(cpu_limit)(ram_usage)(permissions)(total_resources)(self_delegated_bandwidth)(voter_info) )
FC_REFLECT( eosio::chain_apis::read_only::get_code_results, (account_name)(code_hash)(wast)(wasm)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::get_abi_results, (account_name)(abi) )
FC_REFLECT( eosio::chain_apis::read_only::get_account_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::get_code_params, (account_name)(code_as_wasm) )
FC_REFLECT( eosio::chain_apis::read_only::get_abi_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::producer_info, (producer_name) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_params, (code)(action)(args) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_result, (binargs) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_params, (code)(action)(binargs) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_result, (args) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_params, (transaction)(available_keys) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_result, (required_keys) )
