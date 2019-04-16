/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <boost/algorithm/string.hpp>

#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/generated_transaction_object.hpp>

#include <eosio/plugins_common/http_request_handlers.hpp>
#include <eosio/plugins_common/chain_utils.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/names.hpp>

#include <fc/io/json.hpp>

#include <eosio/chain_api_plugin/chain_api_plugin_params.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin_results.hpp>

namespace eosio {
static appbase::abstract_plugin& _chain_api_plugin = app().register_plugin<chain_api_plugin>();

class chain_api_plugin_impl {
public:
    chain_api_plugin_impl(chain::controller& chain_controller, const fc::microseconds& abi_serializer_max_time, bool shorten_abi_errors)
      : chain_controller_(chain_controller), abi_serializer_max_time_(abi_serializer_max_time), shorten_abi_errors_(shorten_abi_errors) {}

    get_account_results get_account( const get_account_params& params )const;
    chain::symbol extract_core_symbol() const;
    get_code_results get_code( const get_code_params& params )const;
    get_code_hash_results get_code_hash( const get_code_hash_params& params )const;
    get_abi_results get_abi( const get_abi_params& params )const;
    get_raw_code_and_abi_results get_raw_code_and_abi( const get_raw_code_and_abi_params& params)const;
    get_raw_abi_results get_raw_abi( const get_raw_abi_params& params)const;

    /*
    * each name can be either domain name or full name.
    * full name can be in 2 forms: username@domain and username@@account (note double @)
    */

    /*
    * 1. throws if any name is invalid or can't be resolved
    * 2. domain resolves either to {resolved_domain: account}, or {resolved_domain: 0} if unlinked
    * 3. username@domain resolves to {resolved_domain: accountD, resolved_username: accountU}
    * 4. username@@account resolves to {resolved_username: account}
    */
    resolve_names_results resolve_names(const resolve_names_params& params) const;
    abi_json_to_bin_result abi_json_to_bin( const abi_json_to_bin_params& params )const;
    abi_bin_to_json_result abi_bin_to_json( const abi_bin_to_json_params& params )const;
    get_required_keys_result get_required_keys( const get_required_keys_params& params)const;
    get_transaction_id_result get_transaction_id( const get_transaction_id_params& params)const;
    get_table_rows_result get_table_rows( const get_table_rows_params& params )const;
    get_table_by_scope_result get_table_by_scope( const get_table_by_scope_params& params )const;
    std::vector<chain::asset> get_currency_balance( const get_currency_balance_params& params )const;
    fc::variant get_currency_stats( const get_currency_stats_params& params )const;
    get_producers_result get_producers( const get_producers_params& params )const;
    get_producer_schedule_result get_producer_schedule( const get_producer_schedule_params& params )const;
    get_scheduled_transactions_result get_scheduled_transactions( const get_scheduled_transactions_params& params ) const;

private:
   get_table_rows_result walk_table_row_range(const get_table_rows_params& p,
                                              cyberway::chaindb::find_info& itr, cyberway::chaindb::primary_key_t end_pk) const;


    const chain::controller& chain_controller_;
    const fc::microseconds abi_serializer_max_time_;
    bool  shorten_abi_errors_ = true;
};


chain_api_plugin::chain_api_plugin(){}
chain_api_plugin::~chain_api_plugin(){}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

get_abi_results chain_api_plugin_impl::get_abi( const get_abi_params& params )const {
   get_abi_results result;
   result.account_name = params.account_name;
   auto& d = chain_controller_.chaindb();
   const auto& accnt  = d.get<chain::account_object, chain::by_name>( params.account_name );

   chain::abi_def abi;
   if( chain::abi_serializer::to_abi(accnt.abi, abi) ) {
      result.abi = std::move(abi);
   }

   return result;
}

get_code_results chain_api_plugin_impl::get_code( const get_code_params& params )const {
   get_code_results result;
   result.account_name = params.account_name;
   auto& d = chain_controller_.chaindb();
   const auto& accnt  = d.get<chain::account_object,chain::by_name>( params.account_name );

   EOS_ASSERT( params.code_as_wasm, chain::unsupported_feature, "Returning WAST from get_code is no longer supported" );

   if( accnt.code.size() ) {
      result.wasm = accnt.code;
      result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   }

   chain::abi_def abi;
   if( chain::abi_serializer::to_abi(accnt.abi, abi) ) {
      result.abi = std::move(abi);
   }

   return result;
}

get_code_hash_results chain_api_plugin_impl::get_code_hash( const get_code_hash_params& params )const {
   get_code_hash_results result;
   result.account_name = params.account_name;
   auto& d = chain_controller_.chaindb();
   const auto& accnt  = d.get<chain::account_object, chain::by_name>( params.account_name );

   if( accnt.code.size() ) {
      result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   }

   return result;
}

get_raw_code_and_abi_results chain_api_plugin_impl::get_raw_code_and_abi( const get_raw_code_and_abi_params& params)const {
   get_raw_code_and_abi_results result;
   result.account_name = params.account_name;

   auto& d = chain_controller_.chaindb();
   const auto& accnt = d.get<chain::account_object, chain::by_name>(params.account_name);
   result.wasm = fc::base64_encode({accnt.code.begin(), accnt.code.end()});
   result.abi = fc::base64_encode({accnt.abi.begin(), accnt.abi.end()});

   return result;
}

get_raw_abi_results chain_api_plugin_impl::get_raw_abi( const get_raw_abi_params& params )const {
   get_raw_abi_results result;
   result.account_name = params.account_name;

   auto& d = chain_controller_.chaindb();
   const auto& accnt = d.get<chain::account_object, chain::by_name>(params.account_name);
   result.abi_hash = fc::sha256::hash( accnt.abi.data(), accnt.abi.size() );
   result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   if( !params.abi_hash || *params.abi_hash != result.abi_hash )
      result.abi = fc::base64_encode({accnt.abi.begin(), accnt.abi.end()});

   return result;
}

get_account_results chain_api_plugin_impl::get_account( const get_account_params& params )const {
   get_account_results result;
   result.account_name = params.account_name;

   auto& d = chain_controller_.chaindb();
   const auto& rm = chain_controller_.get_resource_limits_manager();

   result.head_block_num  = chain_controller_.head_block_num();
   result.head_block_time = chain_controller_.head_block_time();

   const auto& a = chain_controller_.get_account(result.account_name);

   result.privileged       = a.privileged;
   result.last_code_update = a.last_code_update;
   result.created          = a.creation_date;

   int64_t now = chain_controller_.pending_block_time().sec_since_epoch();
   //TODO:
   result.net_limit = {};
   result.cpu_limit = {};

   result.ram_usage = rm.get_account_usage( result.account_name )[chain::resource_limits::ram_code];

   auto table = d.get_table<chain::permission_object>();
   auto permissions = table.get_index<chain::by_owner>();
   auto perm = permissions.lower_bound( boost::make_tuple( params.account_name ) );
   while( perm != permissions.end() && perm->owner == params.account_name ) {
      /// TODO: lookup perm->parent name
      chain::name parent;

      // Don't lookup parent if null
      if( perm->parent._id ) {
         const auto* p = d.find<chain::permission_object, by_id>( perm->parent );
         if( p ) {
            EOS_ASSERT(perm->owner == p->owner, chain::invalid_parent_permission, "Invalid parent permission");
            parent = p->name;
         }
      }

      result.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
      ++perm;
   }

   const auto& code_account = chain_controller_.chaindb().get<chain::account_object, chain::by_name>( chain::config::system_account_name );

//   TODO: Move out this logic in correct place
//   abi_def abi;
//   if( abi_serializer::to_abi(code_account.abi, abi) ) {
//      abi_serializer abis( abi, abi_serializer_max_time );
//
//      const auto token_code = N(cyber.token);
//
//      auto core_symbol = extract_core_symbol();
//
//      if (params.expected_core_symbol.valid())
//         core_symbol = *(params.expected_core_symbol);
//
//      const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( token_code, params.account_name, N(accounts) ));
//      if( t_id != nullptr ) {
//         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//         auto it = idx.find(boost::make_tuple( t_id->id, core_symbol.to_symbol_code() ));
//         if( it != idx.end() && it->value.size() >= sizeof(asset) ) {
//            asset bal;
//            fc::datastream<const char *> ds(it->value.data(), it->value.size());
//            fc::raw::unpack(ds, bal);
//
//            if( bal.get_symbol().valid() && bal.get_symbol() == core_symbol ) {
//               result.core_liquid_balance = bal;
//            }
//         }
//      }
//
//      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, params.account_name, N(userres) ));
//      if (t_id != nullptr) {
//         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
//         if ( it != idx.end() ) {
//            vector<char> data;
//            copy_inline_row(*it, data);
//            result.total_resources = abis.binary_to_variant( "user_resources", data, abi_serializer_max_time, shorten_abi_errors );
//         }
//      }
//
//      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, params.account_name, N(delband) ));
//      if (t_id != nullptr) {
//         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
//         if ( it != idx.end() ) {
//            vector<char> data;
//            copy_inline_row(*it, data);
//            result.self_delegated_bandwidth = abis.binary_to_variant( "delegated_bandwidth", data, abi_serializer_max_time, shorten_abi_errors );
//         }
//      }
//
//      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, params.account_name, N(refunds) ));
//      if (t_id != nullptr) {
//         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
//         if ( it != idx.end() ) {
//            vector<char> data;
//            copy_inline_row(*it, data);
//            result.refund_request = abis.binary_to_variant( "refund_request", data, abi_serializer_max_time, shorten_abi_errors );
//         }
//      }
//
//      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, config::system_account_name, N(voters) ));
//      if (t_id != nullptr) {
//         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
//         if ( it != idx.end() ) {
//            vector<char> data;
//            copy_inline_row(*it, data);
//            result.voter_info = abis.binary_to_variant( "voter_info", data, abi_serializer_max_time, shorten_abi_errors );
//         }
//      }
//   }
   return result;
}

resolve_names_results chain_api_plugin_impl::resolve_names(const resolve_names_params& p) const {
    resolve_names_results r;

    auto set_domain = [&](const auto& n, resolve_names_item& item) {
        chain::validate_domain_name(n);
        item.resolved_domain = chain_controller_.get_domain(n).linked_to;
    };

    // don't limit names count, but prevent from running too long
    auto timeout = fc::time_point::now() + fc::microseconds(1000 * 10); // 10ms max time

    for (const auto& n: p) { try {
        resolve_names_item item; // TODO: restrict doubles or cache names
        auto at = n.find('@');
        if (at == string::npos) {
            set_domain(n, item);
        } else {
            auto tail_pos = at + 1;
            auto at2 = n.find('@', tail_pos);
            bool at_acc = at2 == tail_pos;
            if (at_acc) {
                tail_pos++;
                at2 = n.find('@', tail_pos);
            }
            EOS_ASSERT(at2 == string::npos, chain::username_type_exception, "Unknown name format: excess `@` symbol");
            auto username = n.substr(0, at);
            bool have_username = username.size() > 0;
            if (have_username) {
                chain::validate_username(username);
            }

            auto tail = n.substr(tail_pos, n.length() - tail_pos);
            if (!at_acc) {
                set_domain(tail, item);
            }
            if (have_username) {
                auto scope = at_acc ? chain::name(tail) : *item.resolved_domain;
                item.resolved_username = chain_controller_.get_username(scope, username).owner;
            }
        }
        r.push_back(item);
        if (fc::time_point::now() > timeout) {
            break;   // early exit if takes too much time
        }
    } EOS_RETHROW_EXCEPTIONS(chain::domain_name_type_exception, "Can't resolve name: ${n}", ("n", n)) }   // TODO: use same exception as thrown
    return r;
}


static fc::variant action_abi_to_variant( const chain::abi_def& abi, chain::type_name action_type ) {
   fc::variant v;
   auto it = std::find_if(abi.structs.begin(), abi.structs.end(), [&](auto& x){return x.name == action_type;});
   if( it != abi.structs.end() )
      to_variant( it->fields,  v );
   return v;
}

abi_json_to_bin_result chain_api_plugin_impl::abi_json_to_bin( const abi_json_to_bin_params& params )const try {
   abi_json_to_bin_result result;
   const auto code_account = chain_controller_.chaindb().find<chain::account_object, chain::by_name>( params.code );
   EOS_ASSERT(code_account != nullptr, chain::contract_query_exception, "Contract can't be found ${contract}", ("contract", params.code));

   chain::abi_def abi;
   if( chain::abi_serializer::to_abi(code_account->abi, abi) ) {
      chain::abi_serializer abis( abi, abi_serializer_max_time_ );
      auto action_type = abis.get_action_type(params.action);
      EOS_ASSERT(!action_type.empty(), chain::action_validate_exception, "Unknown action ${action} in contract ${contract}", ("action", params.action)("contract", params.code));
      try {
         result.binargs = abis.variant_to_binary( action_type, params.args, abi_serializer_max_time_, shorten_abi_errors_);
      } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                "'${args}' is invalid args for action '${action}' code '${code}'. expected '${proto}'",
                                ("args", params.args)("action", params.action)("code", params.code)("proto", action_abi_to_variant(abi, action_type)))
   } else {
      EOS_ASSERT(false, chain::abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
   }
   return result;
} FC_RETHROW_EXCEPTIONS( warn, "code: ${code}, action: ${action}, args: ${args}",
                         ("code", params.code)( "action", params.action )( "args", params.args ))

abi_bin_to_json_result chain_api_plugin_impl::abi_bin_to_json( const abi_bin_to_json_params& params )const {
   abi_bin_to_json_result result;
   const auto& code_account = chain_controller_.chaindb().get<chain::account_object, chain::by_name>( params.code );
   chain::abi_def abi;
   if( chain::abi_serializer::to_abi(code_account.abi, abi) ) {
      chain::abi_serializer abis( abi, abi_serializer_max_time_);
      result.args = abis.binary_to_variant( abis.get_action_type( params.action ), params.binargs, abi_serializer_max_time_, shorten_abi_errors_);
   } else {
      EOS_ASSERT(false, chain::abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
   }
   return result;
}

get_required_keys_result chain_api_plugin_impl::get_required_keys( const get_required_keys_params& params )const {
   chain::transaction pretty_input;
   auto resolver = make_resolver(chain_controller_.chaindb(), abi_serializer_max_time_);
   try {
      chain::abi_serializer::from_variant(params.transaction, pretty_input, resolver, abi_serializer_max_time_);
   } EOS_RETHROW_EXCEPTIONS(chain::transaction_type_exception, "Invalid transaction")

   auto required_keys_set = chain_controller_.get_authorization_manager().get_required_keys( pretty_input, params.available_keys, fc::seconds( pretty_input.delay_sec ));
   get_required_keys_result result;
   result.required_keys = required_keys_set;
   return result;
}

get_transaction_id_result chain_api_plugin_impl::get_transaction_id( const get_transaction_id_params& params)const {
   return params.id();
}


chain::symbol chain_api_plugin_impl::extract_core_symbol() const {
   chain::symbol core_symbol(0);

   // The following code makes assumptions about the contract deployed on cyber account (i.e. the system contract) and how it stores its data.
// TODO: Removed by CyberWay
//   const auto& d = db_controller_.db();
//   const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
//      boost::make_tuple(config::system_account_name, config::system_account_name, N(rammarket)));
//   if( t_id != nullptr ) {
//      const auto &idx = d.get_index<key_value_index, by_scope_primary>();
//      auto it = idx.find(boost::make_tuple( t_id->id, eosio::chain::string_to_symbol_c(4,"RAMCORE") ));
//      if( it != idx.end() ) {
//         detail::ram_market_exchange_state_t ram_market_exchange_state;
//
//         fc::datastream<const char *> ds( it->value.data(), it->value.size() );
//
//         try {
//            fc::raw::unpack(ds, ram_market_exchange_state);
//         } catch( ... ) {
//            return core_symbol;
//         }
//
//         if( ram_market_exchange_state.core_symbol.get_symbol().valid() ) {
//            core_symbol = ram_market_exchange_state.core_symbol.get_symbol();
//         }
//      }
//   }

   return core_symbol;
}

get_table_rows_result chain_api_plugin_impl::get_table_rows( const get_table_rows_params& p )const {
   auto& chaindb = chain_controller_.chaindb();

   const cyberway::chaindb::index_request request{p.code, p.scope, p.table, p.index};

   if (p.reverse && *p.reverse) {
       // TODO: implement rbegin end rend methods in mongo driver https://github.com/GolosChain/cyberway/issues/446
       EOS_THROW(cyberway::chaindb::driver_unsupported_operation_exception, "Backward iteration through table not supported yet");
   } else {
       auto begin = p.lower_bound == fc::variant() ? chaindb.begin(request) : chaindb.lower_bound(request, p.lower_bound);
       const auto end_pk = p.upper_bound == fc::variant() ? cyberway::chaindb::end_primary_key : chaindb.upper_bound(request, p.upper_bound).pk;
       return walk_table_row_range(p, begin, end_pk);
   }

}

get_table_rows_result chain_api_plugin_impl::walk_table_row_range(const get_table_rows_params& p, cyberway::chaindb::find_info& itr, cyberway::chaindb::primary_key_t end_pk) const {
    get_table_rows_result result;

    auto cur_time = fc::time_point::now();
    const auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time

    auto& chaindb = chain_controller_.chaindb();
    cyberway::chaindb::cursor_request cursor{p.code, itr.cursor};

    for(unsigned int count = 0;
        cur_time <= end_time && count < p.limit && itr.pk != end_pk;
        itr.pk = chaindb.next(cursor), ++count, cur_time = fc::time_point::now()) {
        if (p.show_payer && *p.show_payer) {
            const auto object = chaindb.object_at_cursor(cursor);
            auto value = fc::mutable_variant_object()("data", object.value)("payer", object.service.payer);
            result.rows.push_back(std::move(value));
        } else {
            result.rows.push_back(chaindb.value_at_cursor(cursor));
        }
    }

    result.more = itr.pk != end_pk;
    return result;
}


get_table_by_scope_result chain_api_plugin_impl::get_table_by_scope( const get_table_by_scope_params& p )const {
    get_table_by_scope_result result;
// TODO: Removed by CyberWay
//   const auto& d = db_controller_.db();
//
//   const auto& idx = d.get_index<chain::table_id_multi_index, chain::by_code_scope_table>();
//   auto lower_bound_lookup_tuple = std::make_tuple( p.code.value, std::numeric_limits<uint64_t>::lowest(), p.table.value );
//   auto upper_bound_lookup_tuple = std::make_tuple( p.code.value, std::numeric_limits<uint64_t>::max(),
//                                                    (p.table.empty() ? std::numeric_limits<uint64_t>::max() : p.table.value) );
//
//   if( p.lower_bound.size() ) {
//      uint64_t scope = convert_to_type<uint64_t>(p.lower_bound, "lower_bound scope");
//      std::get<1>(lower_bound_lookup_tuple) = scope;
//   }
//
//   if( p.upper_bound.size() ) {
//      uint64_t scope = convert_to_type<uint64_t>(p.upper_bound, "upper_bound scope");
//      std::get<1>(upper_bound_lookup_tuple) = scope;
//   }
//
//   if( upper_bound_lookup_tuple < lower_bound_lookup_tuple )
//      return result;
//
//   auto walk_table_range = [&]( auto itr, auto end_itr ) {
//      auto cur_time = fc::time_point::now();
//      auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time
//      for( unsigned int count = 0; cur_time <= end_time && count < p.limit && itr != end_itr; ++itr, cur_time = fc::time_point::now() ) {
//         if( p.table && itr->table != p.table ) continue;
//
//         result.rows.push_back( {itr->code, itr->scope, itr->table, itr->payer, itr->count} );
//
//         ++count;
//      }
//      if( itr != end_itr ) {
//         result.more = string(itr->scope);
//      }
//   };
//
//   auto lower = idx.lower_bound( lower_bound_lookup_tuple );
//   auto upper = idx.upper_bound( upper_bound_lookup_tuple );
//   if( p.reverse && *p.reverse ) {
//      walk_table_range( boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower) );
//   } else {
//      walk_table_range( lower, upper );
//   }

   return result;
}

std::vector<chain::asset> chain_api_plugin_impl::get_currency_balance( const get_currency_balance_params& p )const {
    vector<chain::asset> results;
    auto& chaindb = chain_controller_.chaindb();

    const cyberway::chaindb::index_request request{p.code, p.account, N(accounts), cyberway::chaindb::names::primary_index};

    auto accounts_it = chaindb.begin(request);

    const auto next_request = cyberway::chaindb::cursor_request{p.code, accounts_it.cursor};

    for (; accounts_it.pk != cyberway::chaindb::end_primary_key; accounts_it.pk = chaindb.next(next_request)) {

        const auto value = chaindb.value_at_cursor({p.code, accounts_it.cursor});

        const auto balance_object = value["balance"];
        eosio::chain::asset asset_value;

        fc::from_variant(balance_object, asset_value);

        if( !p.symbol || boost::iequals(asset_value.symbol_name(), *p.symbol) ) {
            results.push_back(asset_value);
        }

        // return false if we are looking for one and found it, true otherwise
        if (p.symbol && boost::iequals(asset_value.symbol_name(), *p.symbol)) {
            return results;
        }
    }

    return results;
}

fc::variant chain_api_plugin_impl::get_currency_stats( const get_currency_stats_params& p ) const {
    const chain::name scope = eosio::chain::string_to_symbol(0, boost::algorithm::to_upper_copy(p.symbol).c_str()) >> 8;
    auto& chaindb = chain_controller_.chaindb();
    auto itr = chaindb.begin({p.code, scope, N(stat), cyberway::chaindb::names::primary_index});

    if (itr.pk == cyberway::chaindb::end_primary_key) {
        return {};
    }

    const auto currency_stat_object = chaindb.value_at_cursor({p.code, itr.cursor});

    chain::asset supply;
    fc::from_variant(currency_stat_object["supply"], supply);

    chain::asset max_supply;
    fc::from_variant(currency_stat_object["max_supply"], max_supply);

    return fc::mutable_variant_object{p.symbol, fc::variant{get_currency_stats_result{supply, max_supply, chain::name(currency_stat_object["issuer"].as_string())}}};
}

// TODO: move this and similar functions to a header. Copied from wasm_interface.cpp.
// TODO: fix strict aliasing violation
static float64_t to_softfloat64( double d ) {
   return *reinterpret_cast<float64_t*>(&d);
}

fc::variant get_global_row( const chain::database& db, const chain::abi_def& abi, const chain::abi_serializer& abis, const fc::microseconds& , bool ) {
//   const auto table_type = get_index_type(abi, N(global));
//   EOS_ASSERT(table_type == read_only::KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table global", ("type",table_type));

// TODO: Removed by CyberWay
//   const auto* const table_id = db_controller_.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(config::system_account_name, config::system_account_name, N(global)));
//   EOS_ASSERT(table_id, chain::contract_table_query_exception, "Missing table global");
//
//   const auto& kv_index = db_controller_.get_index<key_value_index, by_scope_primary>();
//   const auto it = kv_index.find(boost::make_tuple(table_id->id, N(global)));
//   EOS_ASSERT(it != kv_index.end(), chain::contract_table_query_exception, "Missing row in table global");
//
//   vector<char> data;
//   read_only::copy_inline_row(*it, data);
//   return abis.binary_to_variant(abis.get_table_type(N(global)), data, abi_serializer_max_time_ms, shorten_abi_errors );
   return fc::variant();
}

get_producers_result chain_api_plugin_impl::get_producers( const get_producers_params& p ) const {
//   const chain::abi_def abi = eosio::get_abi(chain_controller_, chain::config::system_account_name);
//   const auto table_type = get_index_type(abi, N(producers));
//   const chain::abi_serializer abis{ abi, abi_serializer_max_time_ };
 //  EOS_ASSERT(table_type == KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table producers", ("type",table_type));

// TODO: Removed by CyberWay
//   const auto& d = db_controller_.db();
//   const auto lower = name{p.lower_bound};
//
//   static const uint8_t secondary_index_num = 0;
//   const auto* const table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
//           boost::make_tuple(config::system_account_name, config::system_account_name, N(producers)));
//   const auto* const secondary_table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
//           boost::make_tuple(config::system_account_name, config::system_account_name, N(producers) | secondary_index_num));
//   EOS_ASSERT(table_id && secondary_table_id, chain::contract_table_query_exception, "Missing producers table");
//
//   const auto& kv_index = d.get_index<key_value_index, by_scope_primary>();
//   const auto& secondary_index = d.get_index<index_double_index>().indices();
//   const auto& secondary_index_by_primary = secondary_index.get<by_primary>();
//   const auto& secondary_index_by_secondary = secondary_index.get<by_secondary>();

   get_producers_result result;
   const auto stopTime = fc::time_point::now() + fc::microseconds(1000 * 10); // 10ms
   std::vector<char> data;

// TODO: Removed by CyberWay
//   auto it = [&]{
//      if(lower.value == 0)
//         return secondary_index_by_secondary.lower_bound(
//            boost::make_tuple(secondary_table_id->id, to_softfloat64(std::numeric_limits<double>::lowest()), 0));
//      else
//         return secondary_index.project<by_secondary>(
//            secondary_index_by_primary.lower_bound(
//               boost::make_tuple(secondary_table_id->id, lower.value)));
//   }();
//
//   for( ; it != secondary_index_by_secondary.end() && it->t_id == secondary_table_id->id; ++it ) {
//      if (result.rows.size() >= p.limit || fc::time_point::now() > stopTime) {
//         result.more = name{it->primary_key}.to_string();
//         break;
//      }
//      copy_inline_row(*kv_index.find(boost::make_tuple(table_id->id, it->primary_key)), data);
//      if (p.json)
//         result.rows.emplace_back( abis.binary_to_variant( abis.get_table_type(N(producers)), data, abi_serializer_max_time, shorten_abi_errors ) );
//      else
//         result.rows.emplace_back(fc::variant(data));
//   }
//
//   result.total_producer_vote_weight = get_global_row(d, abi, abis, abi_serializer_max_time, shorten_abi_errors)["total_producer_vote_weight"].as_double();
   return result;
}

get_producer_schedule_result chain_api_plugin_impl::get_producer_schedule( const get_producer_schedule_params& p ) const {
   get_producer_schedule_result result;
// TODO: Removed by CyberWay
//   to_variant(db.active_producers(), result.active);
//   if(!db.pending_producers().producers.empty())
//      to_variant(db.pending_producers(), result.pending);
//   auto proposed = db_controller_.proposed_producers();
//   if(proposed && !proposed->producers.empty())
//      to_variant(*proposed, result.proposed);
   return result;
}

auto find_scheduled_transacions_begin(cyberway::chaindb::chaindb_controller& chaindb, const get_scheduled_transactions_params& p) {
    auto by_delay_index = chaindb.get_index<chain::generated_transaction_object, chain::by_delay>();

    if (p.lower_bound.empty()) {
        return by_delay_index.begin();
    }

    try {
        auto when = chain::time_point::from_iso_string( p.lower_bound );
        return by_delay_index.lower_bound(when);
    } catch (...) {
        auto by_trx_id_index = chaindb.get_index<chain::generated_transaction_object, chain::by_trx_id>();
        auto trx_id = chain::transaction_id_type(p.lower_bound);
        auto trx_by_id_it = by_trx_id_index.find(trx_id);

        EOS_ASSERT(trx_by_id_it != by_trx_id_index.end(), chain::transaction_exception, "Unknown Transaction ID: ${txid}", ("txid", trx_id));

        return by_delay_index.lower_bound(trx_by_id_it->delay_until);
    }
}

get_scheduled_transactions_result chain_api_plugin_impl::get_scheduled_transactions( const get_scheduled_transactions_params& p ) const {
    auto& chaindb = chain_controller_.chaindb();

    auto itr = find_scheduled_transacions_begin(chaindb, p);
    const auto end = chaindb.get_index<chain::generated_transaction_object, chain::by_delay>().end();

    get_scheduled_transactions_result result;

    const auto resolver = make_resolver(chaindb, abi_serializer_max_time_);

    auto time_limit = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time
    for (uint32_t count = 0; itr != end && time_limit > fc::time_point::now() && count < p.limit; ++itr, ++count) {
        auto row = fc::mutable_variant_object()
              ("trx_id", itr->trx_id)
              ("sender", itr->sender)
              ("sender_id", itr->sender_id)
              ("payer", itr->payer)
              ("delay_until", itr->delay_until)
              ("expiration", itr->expiration)
              ("published", itr->published)
        ;

      if (p.json) {
         fc::variant pretty_transaction;

         chain::transaction trx;
         fc::datastream<const char*> ds( itr->packed_trx.data(), itr->packed_trx.size() );
         fc::raw::unpack(ds,trx);

         chain::abi_serializer::to_variant(trx, pretty_transaction, resolver, abi_serializer_max_time_);
         row("transaction", pretty_transaction);
      } else {
         auto packed_transaction = chain::bytes(itr->packed_trx.begin(), itr->packed_trx.end());
         row("transaction", packed_transaction);
      }

      result.transactions.emplace_back(std::move(row));
   }

   if (itr != end) {
      result.more = string(itr->trx_id);
   }

   return result;
}

void chain_api_plugin::plugin_startup() {
    ilog( "starting chain_api_plugin" );

    auto& http = app().get_plugin<http_plugin>();
    auto& chain = app().get_plugin<chain_plugin>();

    my.reset(new chain_api_plugin_impl(chain.chain(), chain.get_abi_serializer_max_time(), !http.verbose_errors()));

    http.add_api({
      CREATE_READ_HANDLER((*my), get_account, 200),
      CREATE_READ_HANDLER((*my), get_code, 200),
      CREATE_READ_HANDLER((*my), get_code_hash, 200),
      CREATE_READ_HANDLER((*my), get_abi, 200),
      CREATE_READ_HANDLER((*my), get_raw_code_and_abi, 200),
      CREATE_READ_HANDLER((*my), get_raw_abi, 200),
      CREATE_READ_HANDLER((*my), get_table_rows, 200),
      CREATE_READ_HANDLER((*my), get_table_by_scope, 200),
      CREATE_READ_HANDLER((*my), get_currency_balance, 200),
      CREATE_READ_HANDLER((*my), get_currency_stats, 200),
      CREATE_READ_HANDLER((*my), get_producers, 200),
      CREATE_READ_HANDLER((*my), get_producer_schedule, 200),
      CREATE_READ_HANDLER((*my), get_scheduled_transactions, 200),
      CREATE_READ_HANDLER((*my), abi_json_to_bin, 200),
      CREATE_READ_HANDLER((*my), abi_bin_to_json, 200),
      CREATE_READ_HANDLER((*my), get_required_keys, 200),
      CREATE_READ_HANDLER((*my), get_transaction_id, 200),
      CREATE_READ_HANDLER((*my), resolve_names, 200)
    });
}

} // namespace api
