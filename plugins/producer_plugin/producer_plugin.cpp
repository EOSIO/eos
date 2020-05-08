// #include <eosio/chain/generated_transaction_object.hpp>
// #include <eosio/chain/global_property_object.hpp>
// #include <eosio/chain/plugin_interface.hpp>
// #include <eosio/chain/snapshot.hpp>
// #include <eosio/chain/thread_utils.hpp>
// #include <eosio/chain/transaction_object.hpp>
// #include <eosio/chain/unapplied_transaction_queue.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
// 
// #include <fc/io/json.hpp>
// #include <fc/log/logger_config.hpp>
// #include <fc/scoped_exit.hpp>
// 
// #include <boost/algorithm/string.hpp>
// #include <boost/asio.hpp>
// #include <boost/date_time/posix_time/posix_time.hpp>
// #include <boost/function_output_iterator.hpp>
// #include <boost/multi_index_container.hpp>
// #include <boost/multi_index/hashed_index.hpp>
// #include <boost/multi_index/member.hpp>
// #include <boost/multi_index/ordered_index.hpp>
// #include <boost/range/adaptor/map.hpp>
// #include <boost/signals2/connection.hpp>
// 
// #include <iostream>
// #include <algorithm>

namespace eosio {

producer_plugin::producer_plugin()
: my(new producer_plugin_impl(app().get_io_service()))
{
}

producer_plugin::~producer_plugin()
{
}

// void producer_plugin::set_program_options( boost::program_options::options_description& command_line_options, boost::program_options::options_description& config_file_options) {
//    auto default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
//    auto private_key_default = std::make_pair(default_priv_key.get_public_key(), default_priv_key );
// 
//    boost::program_options::options_description producer_options;
// 
//    producer_options.add_options()
//          ("enable-stale-production,e", boost::program_options::bool_switch()->notifier([this](bool e){my->_production_enabled = e;}),
//           "Enable block production, even if the chain is stale.")
// 
//          ("pause-on-startup,x", boost::program_options::bool_switch()->notifier([this](bool p){my->_pause_production = p;}),
//           "Start this node in a state where production is paused")
// 
//          ("max-transaction-time", boost::program_options::value<int32_t>()->default_value(30),
//           "Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid")
// 
//          ("max-irreversible-block-age", boost::program_options::value<int32_t>()->default_value( -1 ),
//           "Limits the maximum age (in seconds) of the DPOS Irreversible Block for a chain this node will produce blocks on (use negative value to indicate unlimited)")
// 
//          ("producer-name,p", boost::program_options::value<vector<string>>()->composing()->multitoken(),
//           "ID of producer controlled by this node (e.g. inita; may specify multiple times)")
// 
//          ("private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
//           "(DEPRECATED - Use signature-provider instead) Tuple of [public key, WIF private key] (may specify multiple times)")
// 
//          ("signature-provider", boost::program_options::value<vector<string>>()->composing()->multitoken()->default_value(
//              {default_priv_key.get_public_key().to_string() + "=KEY:" + default_priv_key.to_string()},
//               default_priv_key.get_public_key().to_string() + "=KEY:" + default_priv_key.to_string()),
//           "Key=Value pairs in the form <public-key>=<provider-spec>\n"
//           "Where:\n"
//           "   <public-key>    \tis a string form of a vaild EOSIO public key\n\n"
//           "   <provider-spec> \tis a string in the form <provider-type>:<data>\n\n"
//           "   <provider-type> \tis KEY, or KEOSD\n\n"
//           "   KEY:<data>      \tis a string form of a valid EOSIO private key which maps to the provided public key\n\n"
//           "   KEOSD:<data>    \tis the URL where keosd is available and the approptiate wallet(s) are unlocked")
// 
//          ("keosd-provider-timeout", boost::program_options::value<int32_t>()->default_value(5),
//           "Limits the maximum time (in milliseconds) that is allowed for sending blocks to a keosd provider for signing")
// 
//          ("greylist-account", boost::program_options::value<vector<string>>()->composing()->multitoken(),
//           "account that can not access to extended CPU/NET virtual resources")
// 
//          ("greylist-limit", boost::program_options::value<uint32_t>()->default_value(1000),
//           "Limit (between 1 and 1000) on the multiple that CPU/NET virtual resources can extend during low usage (only enforced subjectively; use 1000 to not enforce any limit)")
// 
//          ("produce-time-offset-us", boost::program_options::value<int32_t>()->default_value(0),
//           "Offset of non last block producing time in microseconds. Valid range 0 .. -block_time_interval.")
// 
//          ("last-block-time-offset-us", boost::program_options::value<int32_t>()->default_value(-200000),
//           "Offset of last block producing time in microseconds. Valid range 0 .. -block_time_interval.")
// 
//          ("cpu-effort-percent", boost::program_options::value<uint32_t>()->default_value(config::default_block_cpu_effort_pct / config::percent_1),
//           "Percentage of cpu block production time used to produce block. Whole number percentages, e.g. 80 for 80%")
// 
//          ("last-block-cpu-effort-percent", boost::program_options::value<uint32_t>()->default_value(config::default_block_cpu_effort_pct / config::percent_1),
//           "Percentage of cpu block production time used to produce last block. Whole number percentages, e.g. 80 for 80%")
// 
//          ("max-block-cpu-usage-threshold-us", boost::program_options::value<uint32_t>()->default_value( 5000 ),
//           "Threshold of CPU block production to consider block full; when within threshold of max-block-cpu-usage block can be produced immediately")
// 
//          ("max-block-net-usage-threshold-bytes", boost::program_options::value<uint32_t>()->default_value( 1024 ),
//           "Threshold of NET block production to consider block full; when within threshold of max-block-net-usage block can be produced immediately")
// 
//          ("max-scheduled-transaction-time-per-block-ms", boost::program_options::value<int32_t>()->default_value(100),
//           "Maximum wall-clock time, in milliseconds, spent retiring scheduled transactions in any block before returning to normal transaction processing.")
// 
//          ("subjective-cpu-leeway-us", boost::program_options::value<int32_t>()->default_value( config::default_subjective_cpu_leeway_us ),
//           "Time in microseconds allowed for a transaction that starts with insufficient CPU quota to complete and cover its CPU usage.")
// 
//          ("incoming-defer-ratio", boost::program_options::value<double>()->default_value(1.0),
//           "ratio between incoming transactions and deferred transactions when both are queued for execution")
// 
//          ("incoming-transaction-queue-size-mb", boost::program_options::value<uint16_t>()->default_value( 1024 ),
//           "Maximum size (in MiB) of the incoming transaction queue. Exceeding this value will subjectively drop transaction with resource exhaustion.")
// 
//          ("producer-threads", boost::program_options::value<uint16_t>()->default_value(config::default_controller_thread_pool_size),
//           "Number of worker threads in producer thread pool")
// 
//          ("snapshots-dir", boost::program_options::value<bfs::path>()->default_value("snapshots"),
//           "the location of the snapshots directory (absolute path or relative to application data dir)");
//    config_file_options.add(producer_options);
// }
// 
// bool producer_plugin::is_producer_key(const chain::public_key_type& key) const {
//    auto private_key_itr = my->_signature_providers.find(key);
//    if(private_key_itr != my->_signature_providers.end())
//       return true;
//    return false;
// }
// 
// chain::signature_type producer_plugin::sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const {
//    if(key != chain::public_key_type()) {
//       auto private_key_itr = my->_signature_providers.find(key);
//       EOS_ASSERT(private_key_itr != my->_signature_providers.end(), producer_priv_key_not_found, "Local producer has no private key in config.ini corresponding to public key ${key}", ("key", key));
// 
//       return private_key_itr->second(digest);
//    }
//    else {
//       return chain::signature_type();
//    }
// }
// 
// void producer_plugin::plugin_initialize(const boost::program_options::variables_map& options) {
//    try {
//       my->chain_plug = app().find_plugin<chain_plugin>();
//       EOS_ASSERT( my->chain_plug, plugin_config_exception, "chain_plugin not found" );
//       my->_options = &options;
//       LOAD_VALUE_SET(options, "producer-name", my->_producers)
// 
//       chain::controller& chain = my->chain_plug->chain();
//       unapplied_transaction_queue::process_mode unapplied_mode =
//          (chain.get_read_mode() != chain::db_read_mode::SPECULATIVE) ? unapplied_transaction_queue::process_mode::non_speculative :
//             my->_producers.empty() ? unapplied_transaction_queue::process_mode::speculative_non_producer :
//                unapplied_transaction_queue::process_mode::speculative_producer;
//       my->_unapplied_transactions.set_mode( unapplied_mode );
// 
//       if( options.count("private-key") )
//       {
//          const std::vector<std::string> key_id_to_wif_pair_strings = options["private-key"].as<std::vector<std::string>>();
//          for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
//          {
//             try {
//                auto key_id_to_wif_pair = dejsonify<std::pair<public_key_type, private_key_type>>(key_id_to_wif_pair_string);
//                my->_signature_providers[key_id_to_wif_pair.first] = make_key_signature_provider(key_id_to_wif_pair.second);
//                auto blanked_privkey = std::string(key_id_to_wif_pair.second.to_string().size(), '*' );
//                wlog("\"private-key\" is DEPRECATED, use \"signature-provider=${pub}=KEY:${priv}\"", ("pub",key_id_to_wif_pair.first)("priv", blanked_privkey));
//             } catch ( fc::exception& e ) {
//                elog("Malformed private key pair");
//             }
//          }
//       }
// 
//       if( options.count("signature-provider") ) {
//          const std::vector<std::string> key_spec_pairs = options["signature-provider"].as<std::vector<std::string>>();
//          for (const auto& key_spec_pair : key_spec_pairs) {
//             try {
//                auto delim = key_spec_pair.find("=");
//                EOS_ASSERT(delim != std::string::npos, plugin_config_exception, "Missing \"=\" in the key spec pair");
//                auto pub_key_str = key_spec_pair.substr(0, delim);
//                auto spec_str = key_spec_pair.substr(delim + 1);
// 
//                auto spec_delim = spec_str.find(":");
//                EOS_ASSERT(spec_delim != std::string::npos, plugin_config_exception, "Missing \":\" in the key spec pair");
//                auto spec_type_str = spec_str.substr(0, spec_delim);
//                auto spec_data = spec_str.substr(spec_delim + 1);
// 
//                auto pubkey = public_key_type(pub_key_str);
// 
//                if (spec_type_str == "KEY") {
//                   my->_signature_providers[pubkey] = make_key_signature_provider(private_key_type(spec_data));
//                } else if (spec_type_str == "KEOSD") {
//                   my->_signature_providers[pubkey] = make_keosd_signature_provider(my, spec_data, pubkey);
//                }
// 
//             } catch (...) {
//                elog("Malformed signature provider: \"${val}\", ignoring!", ("val", key_spec_pair));
//             }
//          }
//       }
// 
//       my->_keosd_provider_timeout_us = fc::milliseconds(options.at("keosd-provider-timeout").as<int32_t>());
// 
//       my->_produce_time_offset_us = options.at("produce-time-offset-us").as<int32_t>();
//       EOS_ASSERT( my->_produce_time_offset_us <= 0 && my->_produce_time_offset_us >= -config::block_interval_us, plugin_config_exception,
//                   "produce-time-offset-us ${o} must be 0 .. -${bi}", ("bi", config::block_interval_us)("o", my->_produce_time_offset_us) );
// 
//       my->_last_block_time_offset_us = options.at("last-block-time-offset-us").as<int32_t>();
//       EOS_ASSERT( my->_last_block_time_offset_us <= 0 && my->_last_block_time_offset_us >= -config::block_interval_us, plugin_config_exception,
//                   "last-block-time-offset-us ${o} must be 0 .. -${bi}", ("bi", config::block_interval_us)("o", my->_last_block_time_offset_us) );
// 
//       uint32_t cpu_effort_pct = options.at("cpu-effort-percent").as<uint32_t>();
//       EOS_ASSERT( cpu_effort_pct >= 0 && cpu_effort_pct <= 100, plugin_config_exception,
//                   "cpu-effort-percent ${pct} must be 0 - 100", ("pct", cpu_effort_pct) );
//       cpu_effort_pct *= config::percent_1;
//       int32_t cpu_effort_offset_us = -EOS_PERCENT( config::block_interval_us, chain::config::percent_100 - cpu_effort_pct );
// 
//       uint32_t last_block_cpu_effort_pct = options.at("last-block-cpu-effort-percent").as<uint32_t>();
//       EOS_ASSERT( last_block_cpu_effort_pct >= 0 && last_block_cpu_effort_pct <= 100, plugin_config_exception,
//                   "last-block-cpu-effort-percent ${pct} must be 0 - 100", ("pct", last_block_cpu_effort_pct) );
//       last_block_cpu_effort_pct *= config::percent_1;
//       int32_t last_block_cpu_effort_offset_us = -EOS_PERCENT( config::block_interval_us, chain::config::percent_100 - last_block_cpu_effort_pct );
// 
//       my->_produce_time_offset_us = std::min( my->_produce_time_offset_us, cpu_effort_offset_us );
//       my->_last_block_time_offset_us = std::min( my->_last_block_time_offset_us, last_block_cpu_effort_offset_us );
// 
//       my->_max_block_cpu_usage_threshold_us = options.at( "max-block-cpu-usage-threshold-us" ).as<uint32_t>();
//       EOS_ASSERT( my->_max_block_cpu_usage_threshold_us < config::block_interval_us, plugin_config_exception,
//                   "max-block-cpu-usage-threshold-us ${t} must be 0 .. ${bi}", ("bi", config::block_interval_us)("t", my->_max_block_cpu_usage_threshold_us) );
// 
//       my->_max_block_net_usage_threshold_bytes = options.at( "max-block-net-usage-threshold-bytes" ).as<uint32_t>();
// 
//       my->_max_scheduled_transaction_time_per_block_ms = options.at("max-scheduled-transaction-time-per-block-ms").as<int32_t>();
// 
//       if( options.at( "subjective-cpu-leeway-us" ).as<int32_t>() != config::default_subjective_cpu_leeway_us ) {
//          chain.set_subjective_cpu_leeway( fc::microseconds( options.at( "subjective-cpu-leeway-us" ).as<int32_t>() ) );
//       }
// 
//       my->_max_transaction_time_ms = options.at("max-transaction-time").as<int32_t>();
// 
//       my->_max_irreversible_block_age_us = fc::seconds(options.at("max-irreversible-block-age").as<int32_t>());
// 
//       auto max_incoming_transaction_queue_size = options.at("incoming-transaction-queue-size-mb").as<uint16_t>() * 1024*1024;
// 
//       EOS_ASSERT( max_incoming_transaction_queue_size > 0, plugin_config_exception,
//                   "incoming-transaction-queue-size-mb ${mb} must be greater than 0", ("mb", max_incoming_transaction_queue_size) );
// 
//       my->_unapplied_transactions.set_max_transaction_queue_size( max_incoming_transaction_queue_size );
// 
//       my->_incoming_defer_ratio = options.at("incoming-defer-ratio").as<double>();
// 
//       auto thread_pool_size = options.at( "producer-threads" ).as<uint16_t>();
//       EOS_ASSERT( thread_pool_size > 0, plugin_config_exception,
//                   "producer-threads ${num} must be greater than 0", ("num", thread_pool_size));
//       my->_thread_pool.emplace( "prod", thread_pool_size );
// 
//       if( options.count( "snapshots-dir" )) {
//          auto sd = options.at( "snapshots-dir" ).as<bfs::path>();
//          if( sd.is_relative()) {
//             my->_snapshots_dir = app().data_dir() / sd;
//             if (!fc::exists(my->_snapshots_dir)) {
//                fc::create_directories(my->_snapshots_dir);
//             }
//          } else {
//             my->_snapshots_dir = sd;
//          }
// 
//          EOS_ASSERT( fc::is_directory(my->_snapshots_dir), snapshot_directory_not_found_exception,
//                      "No such directory '${dir}'", ("dir", my->_snapshots_dir.generic_string()) );
//       }
// 
//       my->_incoming_block_subscription = app().get_channel<incoming::channels::block>().subscribe(
//             [this](const signed_block_ptr& block) {
//          try {
//             my->on_incoming_block(block, {});
//          } LOG_AND_DROP();
//       });
// 
//       my->_incoming_transaction_subscription = app().get_channel<incoming::channels::transaction>().subscribe(
//             [this](const packed_transaction_ptr& trx) {
//          try {
//             my->on_incoming_transaction_async(trx, false, [](const auto&){});
//          } LOG_AND_DROP();
//       });
// 
//       my->_incoming_block_sync_provider = app().get_method<incoming::methods::block_sync>().register_provider(
//             [this](const signed_block_ptr& block, const std::optional<block_id_type>& block_id) {
//          return my->on_incoming_block(block, block_id);
//       });
// 
//       my->_incoming_transaction_async_provider = app().get_method<incoming::methods::transaction_async>().register_provider(
//             [this](const packed_transaction_ptr& trx, bool persist_until_expired, next_function<transaction_trace_ptr> next) -> void {
//          return my->on_incoming_transaction_async(trx, persist_until_expired, next );
//       });
// 
//       if (options.count("greylist-account")) {
//          std::vector<std::string> greylist = options["greylist-account"].as<std::vector<std::string>>();
//          greylist_params param;
//          for (auto &a : greylist) {
//             param.accounts.push_back(account_name(a));
//          }
//          add_greylist_accounts(param);
//       }
// 
//       {
//          uint32_t greylist_limit = options.at("greylist-limit").as<uint32_t>();
//          chain.set_greylist_limit( greylist_limit );
//       }
//    } FC_LOG_AND_RETHROW()
// }
// 
// void producer_plugin::plugin_startup() {
//    try {
//       handle_sighup(); // Sets loggers
// 
//       try {
//       ilog("producer plugin:  plugin_startup() begin");
// 
//       chain::controller& chain = my->chain_plug->chain();
//       EOS_ASSERT( my->_producers.empty() || chain.get_read_mode() == chain::db_read_mode::SPECULATIVE, plugin_config_exception,
//                  "node cannot have any producer-name configured because block production is impossible when read_mode is not \"speculative\"" );
// 
//       EOS_ASSERT( my->_producers.empty() || chain.get_validation_mode() == chain::validation_mode::FULL, plugin_config_exception,
//                  "node cannot have any producer-name configured because block production is not safe when validation_mode is not \"full\"" );
//    
//       EOS_ASSERT( my->_producers.empty() || my->chain_plug->accept_transactions(), plugin_config_exception,
//                  "node cannot have any producer-name configured because no block production is possible with no [api|p2p]-accepted-transactions" );
// 
//       my->_accepted_block_connection.emplace(chain.accepted_block.connect( [this]( const auto& bsp ){ my->on_block( bsp ); } ));
//       my->_accepted_block_header_connection.emplace(chain.accepted_block_header.connect( [this]( const auto& bsp ){ my->on_block_header( bsp ); } ));
//       my->_irreversible_block_connection.emplace(chain.irreversible_block.connect( [this]( const auto& bsp ){ my->on_irreversible_block( bsp->block ); } ));
// 
//       const auto lib_num = chain.last_irreversible_block_num();
//       const auto lib = chain.fetch_block_by_number(lib_num);
//       if (lib) {
//          my->on_irreversible_block(lib);
//       } else {
//          my->_irreversible_block_time = fc::time_point::maximum();
//       }
// 
//       if (!my->_producers.empty()) {
//          ilog("Launching block production for ${n} producers at ${time}.", ("n", my->_producers.size())("time",fc::time_point::now()));
//    
//          if (my->_production_enabled) {
//             if (chain.head_block_num() == 0) {
//                new_chain_banner(chain);
//             }
//          }
//       }
// 
//       my->schedule_production_loop();
//    
//       ilog("producer plugin:  plugin_startup() end");
//       } catch( ... ) {
//          // always call plugin_shutdown, even on exception
//          plugin_shutdown();
//          throw;
//       }
//    } FC_CAPTURE_AND_RETHROW()
// }
// 
// void producer_plugin::plugin_shutdown() {
//    try {
//       my->_timer.cancel();
//    } catch(fc::exception& e) {
//       edump((e.to_detail_string()));
//    }
// 
//    if( my->_thread_pool ) {
//       my->_thread_pool->stop();
//    }
// 
//    app().post( 0, [me = my](){} ); // keep my pointer alive until queue is drained
// }
// 
// void producer_plugin::handle_sighup() {
//    fc::logger::update( logger_name, _log );
//    fc::logger::update( trx_trace_logger_name, _trx_trace_log );
// }
// 
// void producer_plugin::pause() {
//    fc_ilog(_log, "Producer paused.");
//    my->_pause_production = true;
// }
// 
// void producer_plugin::resume() {
//    my->_pause_production = false;
//    // it is possible that we are only speculating because of this policy which we have now changed
//    // re-evaluate that now
//    //
//    if (my->_pending_block_mode == pending_block_mode::speculating) {
//       chain::controller& chain = my->chain_plug->chain();
//       my->_unapplied_transactions.add_aborted( chain.abort_block() );
//       fc_ilog(_log, "Producer resumed. Scheduling production.");
//       my->schedule_production_loop();
//    } else {
//       fc_ilog(_log, "Producer resumed.");
//    }
// }
// 
// bool producer_plugin::paused() const {
//    return my->_pause_production;
// }
// 
// void producer_plugin::update_runtime_options(const runtime_options& options) {
//    chain::controller& chain = my->chain_plug->chain();
//    bool check_speculating = false;
// 
//    if (options.max_transaction_time) {
//       my->_max_transaction_time_ms = *options.max_transaction_time;
//    }
// 
//    if (options.max_irreversible_block_age) {
//       my->_max_irreversible_block_age_us =  fc::seconds(*options.max_irreversible_block_age);
//       check_speculating = true;
//    }
// 
//    if (options.produce_time_offset_us) {
//       my->_produce_time_offset_us = *options.produce_time_offset_us;
//    }
// 
//    if (options.last_block_time_offset_us) {
//       my->_last_block_time_offset_us = *options.last_block_time_offset_us;
//    }
// 
//    if (options.max_scheduled_transaction_time_per_block_ms) {
//       my->_max_scheduled_transaction_time_per_block_ms = *options.max_scheduled_transaction_time_per_block_ms;
//    }
// 
//    if (options.incoming_defer_ratio) {
//       my->_incoming_defer_ratio = *options.incoming_defer_ratio;
//    }
// 
//    if (check_speculating && my->_pending_block_mode == pending_block_mode::speculating) {
//       my->_unapplied_transactions.add_aborted( chain.abort_block() );
//       my->schedule_production_loop();
//    }
// 
//    if (options.subjective_cpu_leeway_us) {
//       chain.set_subjective_cpu_leeway(fc::microseconds(*options.subjective_cpu_leeway_us));
//    }
// 
//    if (options.greylist_limit) {
//       chain.set_greylist_limit(*options.greylist_limit);
//    }
// }
// 
// producer_plugin::runtime_options producer_plugin::get_runtime_options() const {
//    return {
//       my->_max_transaction_time_ms,
//       my->_max_irreversible_block_age_us.count() < 0 ? -1 : my->_max_irreversible_block_age_us.count() / 1'000'000,
//       my->_produce_time_offset_us,
//       my->_last_block_time_offset_us,
//       my->_max_scheduled_transaction_time_per_block_ms,
//       my->chain_plug->chain().get_subjective_cpu_leeway() ?
//             my->chain_plug->chain().get_subjective_cpu_leeway()->count() :
//             fc::optional<int32_t>(),
//       my->_incoming_defer_ratio,
//       my->chain_plug->chain().get_greylist_limit()
//    };
// }
// 
// void producer_plugin::add_greylist_accounts(const greylist_params& params) {
//    chain::controller& chain = my->chain_plug->chain();
//    for (auto &acc : params.accounts) {
//       chain.add_resource_greylist(acc);
//    }
// }
// 
// void producer_plugin::remove_greylist_accounts(const greylist_params& params) {
//    chain::controller& chain = my->chain_plug->chain();
//    for (auto &acc : params.accounts) {
//       chain.remove_resource_greylist(acc);
//    }
// }
// 
// producer_plugin::greylist_params producer_plugin::get_greylist() const {
//    chain::controller& chain = my->chain_plug->chain();
//    greylist_params result;
//    const auto& list = chain.get_resource_greylist();
//    result.accounts.reserve(list.size());
//    for (auto &acc: list) {
//       result.accounts.push_back(acc);
//    }
//    return result;
// }
// 
// producer_plugin::whitelist_blacklist producer_plugin::get_whitelist_blacklist() const {
//    chain::controller& chain = my->chain_plug->chain();
//    return {
//       chain.get_actor_whitelist(),
//       chain.get_actor_blacklist(),
//       chain.get_contract_whitelist(),
//       chain.get_contract_blacklist(),
//       chain.get_action_blacklist(),
//       chain.get_key_blacklist()
//    };
// }
// 
// void producer_plugin::set_whitelist_blacklist(const producer_plugin::whitelist_blacklist& params) {
//    chain::controller& chain = my->chain_plug->chain();
//    if(params.actor_whitelist.valid()) chain.set_actor_whitelist(*params.actor_whitelist);
//    if(params.actor_blacklist.valid()) chain.set_actor_blacklist(*params.actor_blacklist);
//    if(params.contract_whitelist.valid()) chain.set_contract_whitelist(*params.contract_whitelist);
//    if(params.contract_blacklist.valid()) chain.set_contract_blacklist(*params.contract_blacklist);
//    if(params.action_blacklist.valid()) chain.set_action_blacklist(*params.action_blacklist);
//    if(params.key_blacklist.valid()) chain.set_key_blacklist(*params.key_blacklist);
// }
// 
// producer_plugin::integrity_hash_information producer_plugin::get_integrity_hash() const {
//    chain::controller& chain = my->chain_plug->chain();
// 
//    auto reschedule = fc::make_scoped_exit([this](){
//       my->schedule_production_loop();
//    });
// 
//    if (chain.is_building_block()) {
//       // abort the pending block
//       my->_unapplied_transactions.add_aborted( chain.abort_block() );
//    } else {
//       reschedule.cancel();
//    }
// 
//    return {chain.head_block_id(), chain.calculate_integrity_hash()};
// }
// 
// void producer_plugin::create_snapshot(producer_plugin::next_function<producer_plugin::snapshot_information> next) {
//    #warning TODO: Re-enable snapshot generation.
//    auto ex = producer_exception( FC_LOG_MESSAGE( error, "snapshot generation temporarily disabled") );
//    next(ex.dynamic_copy_exception());
//    return;
// 
//    chain::controller& chain = my->chain_plug->chain();
// 
//    auto head_id = chain.head_block_id();
//    const auto& snapshot_path = pending_snapshot::get_final_path(head_id, my->_snapshots_dir);
//    const auto& temp_path     = pending_snapshot::get_temp_path(head_id, my->_snapshots_dir);
// 
//    // maintain legacy exception if the snapshot exists
//    if( fc::is_regular_file(snapshot_path) ) {
//       auto ex = snapshot_exists_exception( FC_LOG_MESSAGE( error, "snapshot named ${name} already exists", ("name", snapshot_path.generic_string()) ) );
//       next(ex.dynamic_copy_exception());
//       return;
//    }
// 
//    auto write_snapshot = [&]( const bfs::path& p ) -> void {
//       auto reschedule = fc::make_scoped_exit([this](){
//          my->schedule_production_loop();
//       });
// 
//       if (chain.is_building_block()) {
//          // abort the pending block
//          my->_unapplied_transactions.add_aborted( chain.abort_block() );
//       } else {
//          reschedule.cancel();
//       }
// 
//       bfs::create_directory( p.parent_path() );
// 
//       // create the snapshot
//       auto snap_out = std::ofstream(p.generic_string(), (std::ios::out | std::ios::binary));
//       auto writer = std::make_shared<ostream_snapshot_writer>(snap_out);
//       chain.write_snapshot(writer);
//       writer->finalize();
//       snap_out.flush();
//       snap_out.close();
//    };
// 
//    // If in irreversible mode, create snapshot and return path to snapshot immediately.
//    if( chain.get_read_mode() == db_read_mode::IRREVERSIBLE ) {
//       try {
//          write_snapshot( temp_path );
// 
//          boost::system::error_code ec;
//          bfs::rename(temp_path, snapshot_path, ec);
//          EOS_ASSERT(!ec, snapshot_finalization_exception,
//                "Unable to finalize valid snapshot of block number ${bn}: [code: ${ec}] ${message}",
//                ("bn", chain.head_block_num())
//                ("ec", ec.value())
//                ("message", ec.message()));
// 
//          next( producer_plugin::snapshot_information{head_id, snapshot_path.generic_string()} );
//       } CATCH_AND_CALL (next);
//       return;
//    }
// 
//    // Otherwise, the result will be returned when the snapshot becomes irreversible.
// 
//    // determine if this snapshot is already in-flight
//    auto& pending_by_id = my->_pending_snapshot_index.get<by_id>();
//    auto existing = pending_by_id.find(head_id);
//    if( existing != pending_by_id.end() ) {
//       // if a snapshot at this block is already pending, attach this requests handler to it
//       pending_by_id.modify(existing, [&next]( auto& entry ){
//          entry.next = [prev = entry.next, next](const fc::static_variant<fc::exception_ptr, producer_plugin::snapshot_information>& res){
//             prev(res);
//             next(res);
//          };
//       });
//    } else {
//       const auto& pending_path = pending_snapshot::get_pending_path(head_id, my->_snapshots_dir);
// 
//       try {
//          write_snapshot( temp_path ); // create a new pending snapshot
// 
//          boost::system::error_code ec;
//          bfs::rename(temp_path, pending_path, ec);
//          EOS_ASSERT(!ec, snapshot_finalization_exception,
//                "Unable to promote temp snapshot to pending for block number ${bn}: [code: ${ec}] ${message}",
//                ("bn", chain.head_block_num())
//                ("ec", ec.value())
//                ("message", ec.message()));
// 
//          my->_pending_snapshot_index.emplace(head_id, next, pending_path.generic_string(), snapshot_path.generic_string());
//       } CATCH_AND_CALL (next);
//    }
// }
// 
// producer_plugin::scheduled_protocol_feature_activations producer_plugin::get_scheduled_protocol_feature_activations() const {
//    return {my->_protocol_features_to_activate};
// }
// 
// void producer_plugin::schedule_protocol_feature_activations( const scheduled_protocol_feature_activations& schedule ) {
//    const chain::controller& chain = my->chain_plug->chain();
//    std::set<digest_type> set_of_features_to_activate( schedule.protocol_features_to_activate.begin(),
//                                                       schedule.protocol_features_to_activate.end() );
//    EOS_ASSERT( set_of_features_to_activate.size() == schedule.protocol_features_to_activate.size(),
//                invalid_protocol_features_to_activate, "duplicate digests" );
//    chain.validate_protocol_features( schedule.protocol_features_to_activate );
//    const auto& pfs = chain.get_protocol_feature_manager().get_protocol_feature_set();
//    for (auto &feature_digest : set_of_features_to_activate) {
//       const auto& pf = pfs.get_protocol_feature(feature_digest);
//       EOS_ASSERT( !pf.preactivation_required, protocol_feature_exception,
//                   "protocol feature requires preactivation: ${digest}",
//                   ("digest", feature_digest));
//    }
//    my->_protocol_features_to_activate = schedule.protocol_features_to_activate;
//    my->_protocol_features_signaled = false;
// }
// 
// fc::variants producer_plugin::get_supported_protocol_features( const get_supported_protocol_features_params& params ) const {
//    fc::variants results;
//    const chain::controller& chain = my->chain_plug->chain();
//    const auto& pfs = chain.get_protocol_feature_manager().get_protocol_feature_set();
//    const auto next_block_time = chain.head_block_time() + fc::milliseconds(config::block_interval_ms);
// 
//    flat_map<digest_type, bool>  visited_protocol_features;
//    visited_protocol_features.reserve( pfs.size() );
// 
//    std::function<bool(const protocol_feature&)> add_feature =
//    [&results, &pfs, &params, next_block_time, &visited_protocol_features, &add_feature]
//    ( const protocol_feature& pf ) -> bool {
//       if( ( params.exclude_disabled || params.exclude_unactivatable ) && !pf.enabled ) return false;
//       if( params.exclude_unactivatable && ( next_block_time < pf.earliest_allowed_activation_time  ) ) return false;
// 
//       auto res = visited_protocol_features.emplace( pf.feature_digest, false );
//       if( !res.second ) return res.first->second;
// 
//       const auto original_size = results.size();
//       for( const auto& dependency : pf.dependencies ) {
//          if( !add_feature( pfs.get_protocol_feature( dependency ) ) ) {
//             results.resize( original_size );
//             return false;
//          }
//       }
// 
//       res.first->second = true;
//       results.emplace_back( pf.to_variant(true) );
//       return true;
//    };
// 
//    for( const auto& pf : pfs ) {
//       add_feature( pf );
//    }
// 
//    return results;
// }
// 
// producer_plugin::get_account_ram_corrections_result producer_plugin::get_account_ram_corrections( const get_account_ram_corrections_params& params ) const {
//    get_account_ram_corrections_result result;
//    const auto& db = my->chain_plug->chain().db();
// 
//    const auto& idx = db.get_index<chain::account_ram_correction_index, chain::by_name>();
//    account_name lower_bound_value{ std::numeric_limits<uint64_t>::lowest() };
//    account_name upper_bound_value{ std::numeric_limits<uint64_t>::max() };
// 
//    if( params.lower_bound ) {
//       lower_bound_value = *params.lower_bound;
//    }
// 
//    if( params.upper_bound ) {
//       upper_bound_value = *params.upper_bound;
//    }
// 
//    if( upper_bound_value < lower_bound_value )
//       return result;
// 
//    auto walk_range = [&]( auto itr, auto end_itr ) {
//       for( unsigned int count = 0;
//            count < params.limit && itr != end_itr;
//            ++itr )
//       {
//          result.rows.push_back( fc::variant( *itr ) );
//          ++count;
//       }
//       if( itr != end_itr ) {
//          result.more = itr->name;
//       }
//    };
// 
//    auto lower = idx.lower_bound( lower_bound_value );
//    auto upper = idx.upper_bound( upper_bound_value );
//    if( params.reverse ) {
//       walk_range( boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower) );
//    } else {
//       walk_range( lower, upper );
//    }
// 
//    return result;
// }

} // namespace eosio
