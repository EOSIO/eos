/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <boost/algorithm/string.hpp>

#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/stake.hpp>

#include <eosio/plugins_common/http_request_handlers.hpp>
#include <eosio/plugins_common/chain_utils.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/typed_name.hpp>

#include <fc/io/json.hpp>

#include <eosio/chain_api_plugin/chain_api_plugin_params.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin_results.hpp>
#include <eosio/chain/stake.hpp>

namespace eosio {
static appbase::abstract_plugin& _chain_api_plugin = app().register_plugin<chain_api_plugin>();

namespace {
    class resource_calculator;
}

class chain_api_plugin_impl {
public:
    chain_api_plugin_impl(chain::controller& chain_controller, const fc::microseconds& abi_serializer_max_time, bool shorten_abi_errors)
      : chain_controller_(chain_controller), abi_serializer_max_time_(abi_serializer_max_time), shorten_abi_errors_(shorten_abi_errors) {}

    get_account_results get_account( const get_account_params& params )const;
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
    std::string get_agent_public_key(const get_agent_public_key_params& params) const;
    get_proxy_status_result get_proxy_status(const get_proxy_status_params& params) const;
    std::vector<uint8_t> get_proxylevel_limits(const get_proxylevel_limits_params& params) const;

private:
   get_table_rows_result walk_table_row_range(const get_table_rows_params& p,
                                              cyberway::chaindb::find_info& itr, cyberway::chaindb::primary_key_t end_pk) const;

    fc::optional<eosio::chain::asset> get_account_core_liquid_balance(const get_account_params& params) const;

    fc::variant get_refunds(chain::account_name account, const resource_calculator& resource_calc) const;
    fc::variant get_payout(chain::account_name account) const;
    std::string get_agent_public_key(chain::account_name account, chain::symbol symbol) const;

    uint32_t get_grantors_count(chain::account_name account, chain::symbol_code token_code) const;
    uint8_t get_proxy_level(chain::account_name account, chain::symbol_code token_code) const;


private:

    chain::controller& chain_controller_;
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
   const auto& accnt  = d.get<chain::account_object>( params.account_name );

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
   const auto& accnt  = d.get<chain::account_object>( params.account_name );

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
   const auto& accnt  = d.get<chain::account_object>( params.account_name );

   if( accnt.code.size() ) {
      result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   }

   return result;
}

get_raw_code_and_abi_results chain_api_plugin_impl::get_raw_code_and_abi( const get_raw_code_and_abi_params& params)const {
   get_raw_code_and_abi_results result;
   result.account_name = params.account_name;

   auto& d = chain_controller_.chaindb();
   const auto& accnt = d.get<chain::account_object>(params.account_name);
   result.wasm = fc::base64_encode({accnt.code.begin(), accnt.code.end()});
   result.abi = fc::base64_encode({accnt.abi.begin(), accnt.abi.end()});

   return result;
}

get_raw_abi_results chain_api_plugin_impl::get_raw_abi( const get_raw_abi_params& params )const {
   get_raw_abi_results result;
   result.account_name = params.account_name;

   auto& d = chain_controller_.chaindb();
   const auto& accnt = d.get<chain::account_object>(params.account_name);
   result.abi_hash = fc::sha256::hash( accnt.abi.data(), accnt.abi.size() );
   result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   if( !params.abi_hash || *params.abi_hash != result.abi_hash )
      result.abi = fc::base64_encode({accnt.abi.begin(), accnt.abi.end()});

   return result;
}

namespace {

    class resource_calculator {
        struct resource_info {
            uint64_t usage;
            chain::resource_limits::ratio part;
            chain::resource_limits::ratio price;
        };

    public:

        resource_calculator(chain::resource_limits_manager& rm, chain::account_name name) 
        : rm_(rm),
          account_(name),
          prices_(rm.get_pricelist()),
          total_stake_(rm.get_account_stake_ratio(fc::time_point::now(), account_, false).numerator),
          //storage_usage_(rm.get_account_storage_usage(name)),
          resources_usage_(rm.get_account_usage(name)),
          resources_info_(chain::resource_limits::resources_num),
          resource_limits_(chain::resource_limits::resources_num),
          resources_stake_parts_(chain::resource_limits::resources_num) {
            init_resources_info();
            init_account_resource_limits();
            init_resources_available_stake_part();
            //init_ram_qouta();
            try {
                account_balance_ = rm.get_account_balance(fc::time_point::now(), account_,prices_, false);
            } catch(...) {
            }
        }

        const account_resource_limit& get_net_limit() const {
            return resource_limits_.at(chain::resource_limits::NET);
        }

        const account_resource_limit& get_cpu_limit() const {
            return resource_limits_.at(chain::resource_limits::CPU);
        }
/*
        int64_t get_ram_usage() const {
            return storage_usage_.ram_usage;
        }

        int64_t get_ram_owned() const {
            return storage_usage_.ram_owned;
        }

        int64_t get_storage_usage() const {
            return storage_usage_.storage_usage;
        }

        int64_t get_storage_owned() const {
            return storage_usage_.storage_owned;
        }
*/

        int64_t get_net_weight() const {
            return resources_stake_parts_.at(chain::resource_limits::NET);
        }

        int64_t get_cpu_weight() const {
            return resources_stake_parts_.at(chain::resource_limits::CPU);
        }

        int64_t get_ram_total_count() const {
            return get_resource_total_count(chain::resource_limits::STORAGE);
        }

        int64_t get_resource_stake_part(uint64_t stake, chain::resource_limits::resource_id code) const {
            const auto part = resources_info_.at(code).part;
            return chain::int_arithmetic::safe_prop(stake, part.numerator, part.denominator);
        }

        int64_t get_resource_total_stake_part(chain::resource_limits::resource_id code) const {
            return get_resource_stake_part(total_stake_, code);
        }

    private:

        void init_resources_info() {
            init_resource_info(chain::resource_limits::NET);
            init_resource_info(chain::resource_limits::CPU);
            init_resource_info(chain::resource_limits::STORAGE);
        }

        void init_resource_info(chain::resource_limits::resource_id code) {
            const auto usage = resources_usage_.at(code);
            const auto price = prices_.at(code);
            const auto weight = rm_.get_account_usage_ratio(account_, code);

            resources_info_[code] = {usage, weight, price};
        }

        void init_account_resource_limits() {
            init_account_resource_limit(chain::resource_limits::NET);
            init_account_resource_limit(chain::resource_limits::CPU);
        }

        void init_account_resource_limit(chain::resource_limits::resource_id code) {
            const int64_t max = get_resource_total_count(code);
            const int64_t usage = resources_usage_[code];
            const int64_t available = max - usage;
            resource_limits_[code] = account_resource_limit{usage, available, max};
        }

        void init_resources_available_stake_part() {
            init_resource_available_stake_part(chain::resource_limits::NET);
            init_resource_available_stake_part(chain::resource_limits::CPU);
        }

        void init_resource_available_stake_part(chain::resource_limits::resource_id code) {
            resources_stake_parts_[code] = get_resource_available_stake_part(code);
        }

        uint64_t get_resource_available_stake_part(chain::resource_limits::resource_id code) const {
            const auto resource_part = resources_info_.at(code).part;
            return chain::int_arithmetic::safe_prop(account_balance_, resource_part.numerator, resource_part.denominator);
        }

        int64_t get_resource_total_count(chain::resource_limits::resource_id code) const {
            const auto stake_part = get_resource_total_stake_part(code);
            return get_resource_count(code, stake_part);
        }

        int64_t get_resource_count(chain::resource_limits::resource_id code, uint64_t stake_part) const {
            const auto price = prices_.at(code);
            return chain::int_arithmetic::safe_prop(stake_part, price.denominator, price.numerator);
        }

    private:

        const chain::resource_limits_manager& rm_;
        chain::account_name account_;
        std::vector<chain::resource_limits::ratio> prices_;
        uint64_t total_stake_;
        uint64_t account_balance_ = 0;
        //chain::resource_limits::account_storage_usage storage_usage_;
        std::vector<uint64_t> resources_usage_;
        std::vector<resource_info> resources_info_;
        std::vector<account_resource_limit> resource_limits_;
        std::vector<int64_t> resources_stake_parts_;
    };
}

get_account_results chain_api_plugin_impl::get_account(const get_account_params& params) const {
    get_account_results result;
    result.account_name = params.account_name;

    auto& db_controller = chain_controller_.chaindb();
    auto& rm = chain_controller_.get_mutable_resource_limits_manager();

    result.head_block_num  = chain_controller_.head_block_num();
    result.head_block_time = chain_controller_.head_block_time();

    const auto& account = chain_controller_.get_account(result.account_name);

    result.privileged       = account.privileged;
    result.last_code_update = account.last_code_update;
    result.created          = account.creation_date;

    auto table = db_controller.get_table<chain::permission_object>();
    auto permissions = table.get_index<chain::by_owner>();
    auto perm = permissions.lower_bound( boost::make_tuple( params.account_name ) );
    while( perm != permissions.end() && perm->owner == params.account_name ) {
      /// TODO: lookup perm->parent name
      eosio::chain::name parent;

      // Don't lookup parent if null
      if( perm->parent._id ) {
         const auto* p = db_controller.find<chain::permission_object>( perm->parent );
         if( p ) {
            EOS_ASSERT(perm->owner == p->owner, eosio::chain::invalid_parent_permission, "Invalid parent permission");
            parent = p->name;
         }
      }

      result.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
      ++perm;
    }

    result.core_liquid_balance = get_account_core_liquid_balance(params);

    resource_calculator resource_calc(rm, params.account_name);

    result.net_limit = resource_calc.get_net_limit();
    result.cpu_limit = resource_calc.get_cpu_limit();
    
    //result.ram_usage = resource_calc.get_ram_usage();
    //result.ram_owned = resource_calc.get_ram_owned();
    //result.storage_usage = resource_calc.get_storage_usage();
    //result.storage_owned = resource_calc.get_storage_owned();

    result.net_weight = resource_calc.get_net_weight();
    result.cpu_weight = resource_calc.get_cpu_weight();

    const std::string total_net_stake_part = chain::asset(resource_calc.get_resource_total_stake_part(chain::resource_limits::NET)).to_string();
    const std::string total_cpu_stake_part = chain::asset(resource_calc.get_resource_total_stake_part(chain::resource_limits::CPU)).to_string();

    result.total_resources = fc::mutable_variant_object() ("owner", params.account_name)
                                                          ("ram_bytes", resource_calc.get_ram_total_count())
                                                          ("net_weight", total_net_stake_part)
                                                          ("cpu_weight", total_cpu_stake_part);

    result.self_delegated_bandwidth = fc::mutable_variant_object() ("from", params.account_name)
                                                                   ("to", params.account_name)
                                                                   ("net_weight", total_net_stake_part)
                                                                   ("cpu_weight", total_cpu_stake_part);

    result.refund_request = get_refunds(params.account_name, resource_calc);

//    result.voter_info = get_account_voter_info(params);
    return result;
}

fc::variant chain_api_plugin_impl::get_refunds(chain::account_name account, const resource_calculator& resource_calc) const {
    const auto payout = get_payout(account);

    if (!payout.is_null()) {
        const auto balance = payout["balance"].as_uint64();
        return fc::mutable_variant_object() ("owner", account)
                                            ("request_time", payout["last_step"].as_string())
                                            ("net_amount", chain::asset(resource_calc.get_resource_stake_part(balance, chain::resource_limits::NET)).to_string())
                                            ("cpu_amount", chain::asset(resource_calc.get_resource_stake_part(balance, chain::resource_limits::CPU)).to_string());
    }
    return {};
}

fc::variant chain_api_plugin_impl::get_payout(chain::account_name account) const {
    auto& chain_db = chain_controller_.chaindb();

    const cyberway::chaindb::index_request request{N(cyber.stake), N(cyber.stake), N(payout), N(payoutacc)};

    auto payouts_it = chain_db.lower_bound(request, fc::mutable_variant_object() ("token_code", chain::symbol(CORE_SYMBOL).to_symbol_code())
                                                                                 ("account", account));

    if (payouts_it.pk != cyberway::chaindb::primary_key::End) {
        return chain_db.value_at_cursor({N(cyber.stake), payouts_it.cursor});
    }
    return fc::variant();
}


fc::optional<eosio::chain::asset> chain_api_plugin_impl::get_account_core_liquid_balance(const get_account_params& params) const {
    static const auto token_code = N(cyber.token);

    const auto core_symbol = params.expected_core_symbol.valid() ? *(params.expected_core_symbol) : eosio::chain::symbol();

    const cyberway::chaindb::index_request request{token_code, params.account_name, N(accounts), cyberway::chaindb::names::primary_index};

    auto& db_controller = chain_controller_.chaindb();

    auto accounts_it = db_controller.begin(request);
    const auto end_it = db_controller.end(request);

    for (; accounts_it.pk != end_it.pk; accounts_it.pk = db_controller.next({token_code, accounts_it.cursor})) {
        const auto value = db_controller.value_at_cursor({token_code, accounts_it.cursor});
        const auto balance_object = value["balance"];
        eosio::chain::asset asset_value;

        fc::from_variant(balance_object, asset_value);

        if (asset_value.get_symbol() == core_symbol) {
            return {asset_value};
        }
    }

    return {};
}

resolve_names_results chain_api_plugin_impl::resolve_names(const resolve_names_params& p) const {
    resolve_names_results r;

    auto set_domain = [&](const auto& n, resolve_names_item& item) {
        cyberway::chain::validate_domain_name(n);
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
                cyberway::chain::validate_username(username);
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
   const auto code_account = chain_controller_.chaindb().find<chain::account_object>( params.code );
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
   const auto& code_account = chain_controller_.chaindb().get<chain::account_object>( params.code );
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

get_table_rows_result chain_api_plugin_impl::get_table_rows( const get_table_rows_params& p )const {
   auto& chaindb = chain_controller_.chaindb();
   cyberway::chaindb::scope_name_t scope = 0;

   try {
       auto table = chaindb.table_by_request({p.code, scope, p.table});
       scope = cyberway::chaindb::scope_name::from_string(table, p.scope).value();
   } catch (...) {
        return get_table_rows_result();
   }

   const cyberway::chaindb::index_request request{p.code, scope, p.table, p.index};

   if (p.reverse && *p.reverse) {
       // TODO: implement rbegin end rend methods in mongo driver https://github.com/GolosChain/cyberway/issues/446
       EOS_THROW(cyberway::chaindb::driver_unsupported_operation_exception, "Backward iteration through table not supported yet");
   } else {
       auto begin = p.lower_bound.is_null() ? chaindb.begin(request) : chaindb.lower_bound(request, p.lower_bound);
       const auto end_pk = p.upper_bound.is_null() ? cyberway::chaindb::primary_key::End : chaindb.upper_bound(request, p.upper_bound).pk;
       return walk_table_row_range(p, begin, end_pk);
   }

}

get_table_rows_result chain_api_plugin_impl::walk_table_row_range(const get_table_rows_params& p, cyberway::chaindb::find_info& itr, cyberway::chaindb::primary_key_t end_pk) const {
    get_table_rows_result result;

    auto cur_time = fc::time_point::now();
    const auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time

    auto& chaindb = chain_controller_.chaindb();
    cyberway::chaindb::cursor_request cursor{p.code, itr.cursor};
    auto index = chaindb.index_at_cursor(cursor);

    for(unsigned int count = 0;
        cur_time <= end_time && count < p.limit && itr.pk != end_pk;
        itr.pk = chaindb.next(cursor), ++count, cur_time = fc::time_point::now()
    ) {
        if (p.show_payer && *p.show_payer) {
            const auto object = chaindb.object_at_cursor(cursor);
            auto value = fc::mutable_variant_object()
                ("data",    object.value)
                ("scope",   cyberway::chaindb::scope_name::from_table(index).to_string())
                ("primary", cyberway::chaindb::primary_key::from_table(index, object.service.pk).to_string())
                ("payer",   chain::account_name(object.service.payer))
                ("size",    object.service.size)
                ("in_ram",  object.service.in_ram);
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

    for (; accounts_it.pk != cyberway::chaindb::primary_key::End; accounts_it.pk = chaindb.next(next_request)) {

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

    if (itr.pk == cyberway::chaindb::primary_key::End) {
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

get_producers_result chain_api_plugin_impl::get_producers( const get_producers_params& p ) const {
    
    static const auto token_code = chain::symbol(CORE_SYMBOL).to_symbol_code();
    
    std::set<chain::account_name> active_producers;
    for (auto& p : chain_controller_.active_producers().producers) {
        active_producers.insert(p.producer_name);
    }
    
    std::vector<fc::variant> rows;
    std::string next_producer;
    
    auto& db = chain_controller_.chaindb();
    auto cands_table = db.get_table<chain::stake_candidate_object>();
    auto cands_idx = cands_table.get_index<chain::stake_candidate_object::by_key>();
    
    auto cand_itr = cands_idx.lower_bound(chain::stake::agent_key(token_code, chain::string_to_name(p.lower_bound.c_str())));
    auto count = 0;
    while ((cand_itr != cands_idx.end()) && (cand_itr->token_code == token_code)) {
        if (count == p.limit) {
            next_producer = cand_itr->account.to_string();
            break;
        }
        auto producer_info = fc::mutable_variant_object()("owner", cand_itr->account)
                                                         ("total_votes", cand_itr->votes)
                                                         ("producer_key", cand_itr->signing_key)
                                                         ("url", "");
        if (active_producers.find(cand_itr->account) != active_producers.end()) {
            producer_info["is_active"] = 1;
            producer_info["last_claim_time"] = fc::time_point();
        }
        rows.emplace_back(producer_info);
        
        ++count;
        ++cand_itr;
    }
    
    const chain::stake_stat_object* stat = db.find<chain::stake_stat_object, by_id>(token_code.value);
    uint64_t total_votes = (stat && (stat->total_votes > 0)) ? stat->total_votes : 0;
    return {rows, total_votes, next_producer};
}

std::string chain_api_plugin_impl::get_agent_public_key(chain::account_name account, chain::symbol symbol) const {
    auto& chaindb = chain_controller_.chaindb();

    const cyberway::chaindb::index_request request{N(), N(), N(stake.agent), N(bykey)};

    const auto it = chaindb.lower_bound(request, fc::mutable_variant_object()("token_code", symbol.to_symbol_code())("account", account));

    if (it.pk != cyberway::chaindb::primary_key::End) {
        return chaindb.value_at_cursor({N(), it.cursor})["signing_key"].as_string();
    }
    return "";
}

std::string chain_api_plugin_impl::get_agent_public_key(const get_agent_public_key_params& params) const {
    return get_agent_public_key(params.account, params.symbol);
}

get_producer_schedule_result chain_api_plugin_impl::get_producer_schedule( const get_producer_schedule_params& p ) const {
   get_producer_schedule_result result;
   to_variant(chain_controller_.active_producers(), result.active);
   if(!chain_controller_.pending_producers().producers.empty())
      to_variant(chain_controller_.pending_producers(), result.pending);
   auto proposed = chain_controller_.proposed_producers();
   if(proposed && !proposed->producers.empty())
      to_variant(*proposed, result.proposed);
   return result;
}

get_proxy_status_result chain_api_plugin_impl::get_proxy_status(const get_proxy_status_params& params) const {
    const auto token_code = params.symbol.to_symbol_code();

    const auto grantors_count = get_grantors_count(params.account, token_code);
    const auto proxy_level = get_proxy_level(params.account, token_code);

    return {params.account, params.symbol, proxy_level, grantors_count};
}

uint32_t chain_api_plugin_impl::get_grantors_count(chain::account_name account, chain::symbol_code token_code) const {
    auto& chaindb = chain_controller_.chaindb();

    auto grants_table = chaindb.get_table<eosio::chain::stake_grant_object>();
    auto grants_idx = grants_table.get_index<eosio::chain::stake_grant_object::by_key>();

    auto grant_itr = grants_idx.lower_bound(eosio::chain::stake::grant_key(token_code, account));

    uint32_t count = 0;
    for (;grant_itr != grants_idx.end() && grant_itr->token_code == token_code && grant_itr->grantor_name == account; ++grant_itr, ++count);
    return count;
}

uint8_t chain_api_plugin_impl::get_proxy_level(chain::account_name account, chain::symbol_code token_code) const {
    auto& chaindb = chain_controller_.chaindb();

    auto agents_table = chaindb.get_table<eosio::chain::stake_agent_object>();
    auto agents_idx = agents_table.get_index<eosio::chain::stake_agent_object::by_key>();

    auto agents_itr = agents_idx.lower_bound(eosio::chain::stake::agent_key(token_code, account));

    EOS_ASSERT(agents_itr != agents_idx.end(), chain::action_validate_exception, "Account ${account} is not an agent", ("account", account));

    return agents_itr->proxy_level;
}

std::vector<uint8_t> chain_api_plugin_impl::get_proxylevel_limits(const get_proxylevel_limits_params& params) const {
    auto& chaindb = chain_controller_.chaindb();

    auto params_table = chaindb.get_table<eosio::chain::stake_param_object>();
    auto stake_param = params_table.get(params.symbol.to_symbol_code());
    return stake_param.max_proxies;
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
              ("payer", chain::account_name(itr.service().payer))
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
      CREATE_READ_HANDLER((*my), get_agent_public_key, 200),
      CREATE_READ_HANDLER((*my), resolve_names, 200),
      CREATE_READ_HANDLER((*my), get_proxy_status, 200),
      CREATE_READ_HANDLER((*my), get_proxylevel_limits, 200)
    });
}

} // namespace api
