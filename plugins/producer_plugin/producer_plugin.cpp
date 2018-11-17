/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/snapshot.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/scoped_exit.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/signals2/connection.hpp>

namespace bmi = boost::multi_index;
using bmi::indexed_by;
using bmi::ordered_non_unique;
using bmi::member;
using bmi::tag;
using bmi::hashed_unique;

using boost::multi_index_container;

using std::string;
using std::vector;
using std::deque;
using boost::signals2::scoped_connection;

// HACK TO EXPOSE LOGGER MAP

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

const fc::string logger_name("producer_plugin");
fc::logger _log;

const fc::string trx_trace_logger_name("transaction_tracing");
fc::logger _trx_trace_log;

namespace eosio {

static appbase::abstract_plugin& _producer_plugin = app().register_plugin<producer_plugin>();

using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

namespace {
   bool failure_is_subjective(const fc::exception& e, bool deadline_is_subjective) {
      auto code = e.code();
      return (code == block_cpu_usage_exceeded::code_value) ||
             (code == block_net_usage_exceeded::code_value) ||
             (code == deadline_exception::code_value && deadline_is_subjective);
   }
}

struct transaction_id_with_expiry {
   transaction_id_type     trx_id;
   fc::time_point          expiry;
};

struct by_id;
struct by_expiry;

using transaction_id_with_expiry_index = multi_index_container<
   transaction_id_with_expiry,
   indexed_by<
      hashed_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, transaction_id_type, trx_id)>,
      ordered_non_unique<tag<by_expiry>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, fc::time_point, expiry)>
   >
>;

enum class pending_block_mode {
   producing,
   speculating
};
#define CATCH_AND_CALL(NEXT)\
   catch ( const fc::exception& err ) {\
      NEXT(err.dynamic_copy_exception());\
   } catch ( const std::exception& e ) {\
      fc::exception fce( \
         FC_LOG_MESSAGE( warn, "rethrow ${what}: ", ("what",e.what())),\
         fc::std_exception_code,\
         BOOST_CORE_TYPEID(e).name(),\
         e.what() ) ;\
      NEXT(fce.dynamic_copy_exception());\
   } catch( ... ) {\
      fc::unhandled_exception e(\
         FC_LOG_MESSAGE(warn, "rethrow"),\
         std::current_exception());\
      NEXT(e.dynamic_copy_exception());\
   }

class producer_plugin_impl : public std::enable_shared_from_this<producer_plugin_impl> {
   public:
      producer_plugin_impl(boost::asio::io_service& io)
      :_timer(io)
      ,_transaction_ack_channel(app().get_channel<compat::channels::transaction_ack>())
      {
      }

      optional<fc::time_point> calculate_next_block_time(const account_name& producer_name, const block_timestamp_type& current_block_time) const;
      void schedule_production_loop();
      void produce_block();
      bool maybe_produce_block();

      boost::program_options::variables_map _options;
      bool     _production_enabled                 = false;
      bool     _pause_production                   = false;
      uint32_t _production_skip_flags              = 0; //eosio::chain::skip_nothing;

      using signature_provider_type = std::function<chain::signature_type(chain::digest_type)>;
      std::map<chain::public_key_type, signature_provider_type> _signature_providers;
      std::set<chain::account_name>                             _producers;
      boost::asio::deadline_timer                               _timer;
      std::map<chain::account_name, uint32_t>                   _producer_watermarks;
      pending_block_mode                                        _pending_block_mode;
      transaction_id_with_expiry_index                          _persistent_transactions;

      int32_t                                                   _max_transaction_time_ms;
      fc::microseconds                                          _max_irreversible_block_age_us;
      int32_t                                                   _produce_time_offset_us = 0;
      int32_t                                                   _last_block_time_offset_us = 0;
      fc::time_point                                            _irreversible_block_time;
      fc::microseconds                                          _keosd_provider_timeout_us;

      time_point _last_signed_block_time;
      time_point _start_time = fc::time_point::now();
      uint32_t   _last_signed_block_num = 0;

      producer_plugin* _self = nullptr;

      incoming::channels::block::channel_type::handle         _incoming_block_subscription;
      incoming::channels::transaction::channel_type::handle   _incoming_transaction_subscription;

      compat::channels::transaction_ack::channel_type&        _transaction_ack_channel;

      incoming::methods::block_sync::method_type::handle        _incoming_block_sync_provider;
      incoming::methods::transaction_async::method_type::handle _incoming_transaction_async_provider;

      transaction_id_with_expiry_index                         _blacklisted_transactions;

      fc::optional<scoped_connection>                          _accepted_block_connection;
      fc::optional<scoped_connection>                          _irreversible_block_connection;

      /*
       * HACK ALERT
       * Boost timers can be in a state where a handler has not yet executed but is not abortable.
       * As this method needs to mutate state handlers depend on for proper functioning to maintain
       * invariants for other code (namely accepting incoming transactions in a nearly full block)
       * the handlers capture a corelation ID at the time they are set.  When they are executed
       * they must check that correlation_id against the global ordinal.  If it does not match that
       * implies that this method has been called with the handler in the state where it should be
       * cancelled but wasn't able to be.
       */
      uint32_t _timer_corelation_id = 0;

      // keep a expected ratio between defer txn and incoming txn
      double _incoming_trx_weight = 0.0;
      double _incoming_defer_ratio = 1.0; // 1:1

      // path to write the snapshots to
      bfs::path _snapshots_dir;


      void on_block( const block_state_ptr& bsp ) {
         if( bsp->header.timestamp <= _last_signed_block_time ) return;
         if( bsp->header.timestamp <= _start_time ) return;
         if( bsp->block_num <= _last_signed_block_num ) return;

         const auto& active_producer_to_signing_key = bsp->active_schedule.producers;

         flat_set<account_name> active_producers;
         active_producers.reserve(bsp->active_schedule.producers.size());
         for (const auto& p: bsp->active_schedule.producers) {
            active_producers.insert(p.producer_name);
         }

         std::set_intersection( _producers.begin(), _producers.end(),
                                active_producers.begin(), active_producers.end(),
                                boost::make_function_output_iterator( [&]( const chain::account_name& producer )
         {
            if( producer != bsp->header.producer ) {
               auto itr = std::find_if( active_producer_to_signing_key.begin(), active_producer_to_signing_key.end(),
                                        [&](const producer_key& k){ return k.producer_name == producer; } );
               if( itr != active_producer_to_signing_key.end() ) {
                  auto private_key_itr = _signature_providers.find( itr->block_signing_key );
                  if( private_key_itr != _signature_providers.end() ) {
                     auto d = bsp->sig_digest();
                     auto sig = private_key_itr->second( d );
                     _last_signed_block_time = bsp->header.timestamp;
                     _last_signed_block_num  = bsp->block_num;

   //                  ilog( "${n} confirmed", ("n",name(producer)) );
                     _self->confirmed_block( { bsp->id, d, producer, sig } );
                  }
               }
            }
         } ) );

         // since the watermark has to be set before a block is created, we are looking into the future to
         // determine the new schedule to identify producers that have become active
         chain::controller& chain = app().get_plugin<chain_plugin>().chain();
         const auto hbn = bsp->block_num;
         auto new_block_header = bsp->header;
         new_block_header.timestamp = new_block_header.timestamp.next();
         new_block_header.previous = bsp->id;
         auto new_bs = bsp->generate_next(new_block_header.timestamp);

         // for newly installed producers we can set their watermarks to the block they became active
         if (new_bs.maybe_promote_pending() && bsp->active_schedule.version != new_bs.active_schedule.version) {
            flat_set<account_name> new_producers;
            new_producers.reserve(new_bs.active_schedule.producers.size());
            for( const auto& p: new_bs.active_schedule.producers) {
               if (_producers.count(p.producer_name) > 0)
                  new_producers.insert(p.producer_name);
            }

            for( const auto& p: bsp->active_schedule.producers) {
               new_producers.erase(p.producer_name);
            }

            for (const auto& new_producer: new_producers) {
               _producer_watermarks[new_producer] = hbn;
            }
         }
      }

      void on_irreversible_block( const signed_block_ptr& lib ) {
         _irreversible_block_time = lib->timestamp.to_time_point();
      }

      template<typename Type, typename Channel, typename F>
      auto publish_results_of(const Type &data, Channel& channel, F f) {
         auto publish_success = fc::make_scoped_exit([&, this](){
            channel.publish(std::pair<fc::exception_ptr, Type>(nullptr, data));
         });

         try {
            auto trace = f();
            if (trace->except) {
               publish_success.cancel();
               channel.publish(std::pair<fc::exception_ptr, Type>(trace->except->dynamic_copy_exception(), data));
            }
            return trace;
         } catch (const fc::exception& e) {
            publish_success.cancel();
            channel.publish(std::pair<fc::exception_ptr, Type>(e.dynamic_copy_exception(), data));
            throw e;
         } catch( const std::exception& e ) {
            publish_success.cancel();
            auto fce = fc::exception(
               FC_LOG_MESSAGE( info, "Caught std::exception: ${what}", ("what",e.what())),
               fc::std_exception_code,
               BOOST_CORE_TYPEID(e).name(),
               e.what()
            );
            channel.publish(std::pair<fc::exception_ptr, Type>(fce.dynamic_copy_exception(),data));
            throw fce;
         } catch( ... ) {
            publish_success.cancel();
            auto fce = fc::unhandled_exception(
               FC_LOG_MESSAGE( info, "Caught unknown exception"),
               std::current_exception()
            );

            channel.publish(std::pair<fc::exception_ptr, Type>(fce.dynamic_copy_exception(), data));
            throw fce;
         }
      };

      void on_incoming_block(const signed_block_ptr& block) {
         fc_dlog(_log, "received incoming block ${id}", ("id", block->id()));

         EOS_ASSERT( block->timestamp < (fc::time_point::now() + fc::seconds(7)), block_from_the_future, "received a block from the future, ignoring it" );


         chain::controller& chain = app().get_plugin<chain_plugin>().chain();

         /* de-dupe here... no point in aborting block if we already know the block */
         auto id = block->id();
         auto existing = chain.fetch_block_by_id( id );
         if( existing ) { return; }

         // abort the pending block
         chain.abort_block();

         // exceptions throw out, make sure we restart our loop
         auto ensure = fc::make_scoped_exit([this](){
            schedule_production_loop();
         });

         // push the new block
         bool except = false;
         try {
            chain.push_block(block);
         } catch ( const guard_exception& e ) {
            app().get_plugin<chain_plugin>().handle_guard_exception(e);
            return;
         } catch( const fc::exception& e ) {
            elog((e.to_detail_string()));
            except = true;
         } catch ( boost::interprocess::bad_alloc& ) {
            chain_plugin::handle_db_exhaustion();
            return;
         }

         if( except ) {
            app().get_channel<channels::rejected_block>().publish( block );
            return;
         }

         if( chain.head_block_state()->header.timestamp.next().to_time_point() >= fc::time_point::now() ) {
            _production_enabled = true;
         }


         if( fc::time_point::now() - block->timestamp < fc::minutes(5) || (block->block_num() % 1000 == 0) ) {
            ilog("Received block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, conf: ${confs}, latency: ${latency} ms]",
                 ("p",block->producer)("id",fc::variant(block->id()).as_string().substr(8,16))
                 ("n",block_header::num_from_id(block->id()))("t",block->timestamp)
                 ("count",block->transactions.size())("lib",chain.last_irreversible_block_num())("confs", block->confirmed)("latency", (fc::time_point::now() - block->timestamp).count()/1000 ) );
         }
      }

      std::deque<std::tuple<packed_transaction_ptr, bool, next_function<transaction_trace_ptr>>> _pending_incoming_transactions;

      void on_incoming_transaction_async(const packed_transaction_ptr& trx, bool persist_until_expired, next_function<transaction_trace_ptr> next) {
         chain::controller& chain = app().get_plugin<chain_plugin>().chain();
         if (!chain.pending_block_state()) {
            _pending_incoming_transactions.emplace_back(trx, persist_until_expired, next);
            return;
         }

         auto block_time = chain.pending_block_state()->header.timestamp.to_time_point();

         auto send_response = [this, &trx, &chain, &next](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& response) {
            next(response);
            if (response.contains<fc::exception_ptr>()) {
               _transaction_ack_channel.publish(std::pair<fc::exception_ptr, packed_transaction_ptr>(response.get<fc::exception_ptr>(), trx));
               if (_pending_block_mode == pending_block_mode::producing) {
                  fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} is REJECTING tx: ${txid} : ${why} ",
                        ("block_num", chain.head_block_num() + 1)
                        ("prod", chain.pending_block_state()->header.producer)
                        ("txid", trx->id())
                        ("why",response.get<fc::exception_ptr>()->what()));
               } else {
                  fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution is REJECTING tx: ${txid} : ${why} ",
                          ("txid", trx->id())
                          ("why",response.get<fc::exception_ptr>()->what()));
               }
            } else {
               _transaction_ack_channel.publish(std::pair<fc::exception_ptr, packed_transaction_ptr>(nullptr, trx));
               if (_pending_block_mode == pending_block_mode::producing) {
                  fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} is ACCEPTING tx: ${txid}",
                          ("block_num", chain.head_block_num() + 1)
                          ("prod", chain.pending_block_state()->header.producer)
                          ("txid", trx->id()));
               } else {
                  fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution is ACCEPTING tx: ${txid}",
                          ("txid", trx->id()));
               }
            }
         };

         auto id = trx->id();
         if( fc::time_point(trx->expiration()) < block_time ) {
            send_response(std::static_pointer_cast<fc::exception>(std::make_shared<expired_tx_exception>(FC_LOG_MESSAGE(error, "expired transaction ${id}", ("id", id)) )));
            return;
         }

         if( chain.is_known_unexpired_transaction(id) ) {
            send_response(std::static_pointer_cast<fc::exception>(std::make_shared<tx_duplicate>(FC_LOG_MESSAGE(error, "duplicate transaction ${id}", ("id", id)) )));
            return;
         }

         auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
         bool deadline_is_subjective = false;
         if (_max_transaction_time_ms < 0 || (_pending_block_mode == pending_block_mode::producing && block_time < deadline) ) {
            deadline_is_subjective = true;
            deadline = block_time;
         }

         try {
            auto trace = chain.push_transaction(std::make_shared<transaction_metadata>(*trx), deadline);
            if (trace->except) {
               if (failure_is_subjective(*trace->except, deadline_is_subjective)) {
                  _pending_incoming_transactions.emplace_back(trx, persist_until_expired, next);
                  if (_pending_block_mode == pending_block_mode::producing) {
                     fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} COULD NOT FIT, tx: ${txid} RETRYING ",
                             ("block_num", chain.head_block_num() + 1)
                             ("prod", chain.pending_block_state()->header.producer)
                             ("txid", trx->id()));
                  } else {
                     fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution COULD NOT FIT tx: ${txid} RETRYING",
                             ("txid", trx->id()));
                  }
               } else {
                  auto e_ptr = trace->except->dynamic_copy_exception();
                  send_response(e_ptr);
               }
            } else {
               if (persist_until_expired) {
                  // if this trx didnt fail/soft-fail and the persist flag is set, store its ID so that we can
                  // ensure its applied to all future speculative blocks as well.
                  _persistent_transactions.insert(transaction_id_with_expiry{trx->id(), trx->expiration()});
               }
               send_response(trace);
            }

         } catch ( const guard_exception& e ) {
            app().get_plugin<chain_plugin>().handle_guard_exception(e);
         } catch ( boost::interprocess::bad_alloc& ) {
            chain_plugin::handle_db_exhaustion();
         } CATCH_AND_CALL(send_response);
      }


      fc::microseconds get_irreversible_block_age() {
         auto now = fc::time_point::now();
         if (now < _irreversible_block_time) {
            return fc::microseconds(0);
         } else {
            return now - _irreversible_block_time;
         }
      }

      bool production_disabled_by_policy() {
         return !_production_enabled || _pause_production || (_max_irreversible_block_age_us.count() >= 0 && get_irreversible_block_age() >= _max_irreversible_block_age_us);
      }

      enum class start_block_result {
         succeeded,
         failed,
         waiting,
         exhausted
      };

      start_block_result start_block(bool &last_block);

      fc::time_point calculate_pending_block_time() const;
      void schedule_delayed_production_loop(const std::weak_ptr<producer_plugin_impl>& weak_this, const block_timestamp_type& current_block_time);
};

void new_chain_banner(const eosio::chain::controller& db)
{
   std::cerr << "\n"
      "*******************************\n"
      "*                             *\n"
      "*   ------ NEW CHAIN ------   *\n"
      "*   -  Welcome to EOSIO!  -   *\n"
      "*   -----------------------   *\n"
      "*                             *\n"
      "*******************************\n"
      "\n";

   if( db.head_block_state()->header.timestamp.to_time_point() < (fc::time_point::now() - fc::milliseconds(200 * config::block_interval_ms)))
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

producer_plugin::producer_plugin()
   : my(new producer_plugin_impl(app().get_io_service())){
      my->_self = this;
   }

producer_plugin::~producer_plugin() {}

void producer_plugin::set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
   auto private_key_default = std::make_pair(default_priv_key.get_public_key(), default_priv_key );

   boost::program_options::options_description producer_options;

   producer_options.add_options()
         ("enable-stale-production,e", boost::program_options::bool_switch()->notifier([this](bool e){my->_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("pause-on-startup,x", boost::program_options::bool_switch()->notifier([this](bool p){my->_pause_production = p;}), "Start this node in a state where production is paused")
         ("max-transaction-time", bpo::value<int32_t>()->default_value(30),
          "Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid")
         ("max-irreversible-block-age", bpo::value<int32_t>()->default_value( -1 ),
          "Limits the maximum age (in seconds) of the DPOS Irreversible Block for a chain this node will produce blocks on (use negative value to indicate unlimited)")
         ("producer-name,p", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "ID of producer controlled by this node (e.g. inita; may specify multiple times)")
         ("private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "(DEPRECATED - Use signature-provider instead) Tuple of [public key, WIF private key] (may specify multiple times)")
         ("signature-provider", boost::program_options::value<vector<string>>()->composing()->multitoken()->default_value({std::string(default_priv_key.get_public_key()) + "=KEY:" + std::string(default_priv_key)}, std::string(default_priv_key.get_public_key()) + "=KEY:" + std::string(default_priv_key)),
          "Key=Value pairs in the form <public-key>=<provider-spec>\n"
          "Where:\n"
          "   <public-key>    \tis a string form of a vaild EOSIO public key\n\n"
          "   <provider-spec> \tis a string in the form <provider-type>:<data>\n\n"
          "   <provider-type> \tis KEY, or KEOSD\n\n"
          "   KEY:<data>      \tis a string form of a valid EOSIO private key which maps to the provided public key\n\n"
          "   KEOSD:<data>    \tis the URL where keosd is available and the approptiate wallet(s) are unlocked")
         ("keosd-provider-timeout", boost::program_options::value<int32_t>()->default_value(5),
          "Limits the maximum time (in milliseconds) that is allowd for sending blocks to a keosd provider for signing")
         ("greylist-account", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "account that can not access to extended CPU/NET virtual resources")
         ("produce-time-offset-us", boost::program_options::value<int32_t>()->default_value(0),
          "offset of non last block producing time in microseconds. Negative number results in blocks to go out sooner, and positive number results in blocks to go out later")
         ("last-block-time-offset-us", boost::program_options::value<int32_t>()->default_value(0),
          "offset of last block producing time in microseconds. Negative number results in blocks to go out sooner, and positive number results in blocks to go out later")
         ("incoming-defer-ratio", bpo::value<double>()->default_value(1.0),
          "ratio between incoming transations and deferred transactions when both are exhausted")
         ("snapshots-dir", bpo::value<bfs::path>()->default_value("snapshots"),
          "the location of the snapshots directory (absolute path or relative to application data dir)")
         ;
   config_file_options.add(producer_options);
}

bool producer_plugin::is_producer_key(const chain::public_key_type& key) const
{
  auto private_key_itr = my->_signature_providers.find(key);
  if(private_key_itr != my->_signature_providers.end())
    return true;
  return false;
}

chain::signature_type producer_plugin::sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const
{
  if(key != chain::public_key_type()) {
    auto private_key_itr = my->_signature_providers.find(key);
    EOS_ASSERT(private_key_itr != my->_signature_providers.end(), producer_priv_key_not_found, "Local producer has no private key in config.ini corresponding to public key ${key}", ("key", key));

    return private_key_itr->second(digest);
  }
  else {
    return chain::signature_type();
  }
}

template<typename T>
T dejsonify(const string& s) {
   return fc::json::from_string(s).as<T>();
}

#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}

static producer_plugin_impl::signature_provider_type
make_key_signature_provider(const private_key_type& key) {
   return [key]( const chain::digest_type& digest ) {
      return key.sign(digest);
   };
}

static producer_plugin_impl::signature_provider_type
make_keosd_signature_provider(const std::shared_ptr<producer_plugin_impl>& impl, const string& url_str, const public_key_type pubkey) {
   fc::url keosd_url;
   if(boost::algorithm::starts_with(url_str, "unix://"))
      //send the entire string after unix:// to http_plugin. It'll auto-detect which part
      // is the unix socket path, and which part is the url to hit on the server
      keosd_url = fc::url("unix", url_str.substr(7), ostring(), ostring(), ostring(), ostring(), ovariant_object(), fc::optional<uint16_t>());
   else
      keosd_url = fc::url(url_str);
   std::weak_ptr<producer_plugin_impl> weak_impl = impl;

   return [weak_impl, keosd_url, pubkey]( const chain::digest_type& digest ) {
      auto impl = weak_impl.lock();
      if (impl) {
         fc::variant params;
         fc::to_variant(std::make_pair(digest, pubkey), params);
         auto deadline = impl->_keosd_provider_timeout_us.count() >= 0 ? fc::time_point::now() + impl->_keosd_provider_timeout_us : fc::time_point::maximum();
         return app().get_plugin<http_client_plugin>().get_client().post_sync(keosd_url, params, deadline).as<chain::signature_type>();
      } else {
         return signature_type();
      }
   };
}

void producer_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   my->_options = &options;
   LOAD_VALUE_SET(options, "producer-name", my->_producers, types::account_name)

   if( options.count("private-key") )
   {
      const std::vector<std::string> key_id_to_wif_pair_strings = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
      {
         try {
            auto key_id_to_wif_pair = dejsonify<std::pair<public_key_type, private_key_type>>(key_id_to_wif_pair_string);
            my->_signature_providers[key_id_to_wif_pair.first] = make_key_signature_provider(key_id_to_wif_pair.second);
            auto blanked_privkey = std::string(std::string(key_id_to_wif_pair.second).size(), '*' );
            wlog("\"private-key\" is DEPRECATED, use \"signature-provider=${pub}=KEY:${priv}\"", ("pub",key_id_to_wif_pair.first)("priv", blanked_privkey));
         } catch ( fc::exception& e ) {
            elog("Malformed private key pair");
         }
      }
   }

   if( options.count("signature-provider") ) {
      const std::vector<std::string> key_spec_pairs = options["signature-provider"].as<std::vector<std::string>>();
      for (const auto& key_spec_pair : key_spec_pairs) {
         try {
            auto delim = key_spec_pair.find("=");
            EOS_ASSERT(delim != std::string::npos, plugin_config_exception, "Missing \"=\" in the key spec pair");
            auto pub_key_str = key_spec_pair.substr(0, delim);
            auto spec_str = key_spec_pair.substr(delim + 1);

            auto spec_delim = spec_str.find(":");
            EOS_ASSERT(spec_delim != std::string::npos, plugin_config_exception, "Missing \":\" in the key spec pair");
            auto spec_type_str = spec_str.substr(0, spec_delim);
            auto spec_data = spec_str.substr(spec_delim + 1);

            auto pubkey = public_key_type(pub_key_str);

            if (spec_type_str == "KEY") {
               my->_signature_providers[pubkey] = make_key_signature_provider(private_key_type(spec_data));
            } else if (spec_type_str == "KEOSD") {
               my->_signature_providers[pubkey] = make_keosd_signature_provider(my, spec_data, pubkey);
            }

         } catch (...) {
            elog("Malformed signature provider: \"${val}\", ignoring!", ("val", key_spec_pair));
         }
      }
   }

   my->_keosd_provider_timeout_us = fc::milliseconds(options.at("keosd-provider-timeout").as<int32_t>());

   my->_produce_time_offset_us = options.at("produce-time-offset-us").as<int32_t>();

   my->_last_block_time_offset_us = options.at("last-block-time-offset-us").as<int32_t>();

   my->_max_transaction_time_ms = options.at("max-transaction-time").as<int32_t>();

   my->_max_irreversible_block_age_us = fc::seconds(options.at("max-irreversible-block-age").as<int32_t>());

   my->_incoming_defer_ratio = options.at("incoming-defer-ratio").as<double>();

   if( options.count( "snapshots-dir" )) {
      auto sd = options.at( "snapshots-dir" ).as<bfs::path>();
      if( sd.is_relative()) {
         my->_snapshots_dir = app().data_dir() / sd;
         if (!fc::exists(my->_snapshots_dir)) {
            fc::create_directories(my->_snapshots_dir);
         }
      } else {
         my->_snapshots_dir = sd;
      }

      EOS_ASSERT( fc::is_directory(my->_snapshots_dir), snapshot_directory_not_found_exception,
                  "No such directory '${dir}'", ("dir", my->_snapshots_dir.generic_string()) );
   }

   my->_incoming_block_subscription = app().get_channel<incoming::channels::block>().subscribe([this](const signed_block_ptr& block){
      try {
         my->on_incoming_block(block);
      } FC_LOG_AND_DROP();
   });

   my->_incoming_transaction_subscription = app().get_channel<incoming::channels::transaction>().subscribe([this](const packed_transaction_ptr& trx){
      try {
         my->on_incoming_transaction_async(trx, false, [](const auto&){});
      } FC_LOG_AND_DROP();
   });

   my->_incoming_block_sync_provider = app().get_method<incoming::methods::block_sync>().register_provider([this](const signed_block_ptr& block){
      my->on_incoming_block(block);
   });

   my->_incoming_transaction_async_provider = app().get_method<incoming::methods::transaction_async>().register_provider([this](const packed_transaction_ptr& trx, bool persist_until_expired, next_function<transaction_trace_ptr> next) -> void {
      return my->on_incoming_transaction_async(trx, persist_until_expired, next );
   });

   if (options.count("greylist-account")) {
      std::vector<std::string> greylist = options["greylist-account"].as<std::vector<std::string>>();
      greylist_params param;
      for (auto &a : greylist) {
         param.accounts.push_back(account_name(a));
      }
      add_greylist_accounts(param);
   }

} FC_LOG_AND_RETHROW() }

void producer_plugin::plugin_startup()
{ try {
   auto& logger_map = fc::get_logger_map();
   if(logger_map.find(logger_name) != logger_map.end()) {
      _log = logger_map[logger_name];
   }

   if( logger_map.find(trx_trace_logger_name) != logger_map.end()) {
      _trx_trace_log = logger_map[trx_trace_logger_name];
   }

   ilog("producer plugin:  plugin_startup() begin");

   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   EOS_ASSERT( my->_producers.empty() || chain.get_read_mode() == chain::db_read_mode::SPECULATIVE, plugin_config_exception,
              "node cannot have any producer-name configured because block production is impossible when read_mode is not \"speculative\"" );

   EOS_ASSERT( my->_producers.empty() || chain.get_validation_mode() == chain::validation_mode::FULL, plugin_config_exception,
              "node cannot have any producer-name configured because block production is not safe when validation_mode is not \"full\"" );


   my->_accepted_block_connection.emplace(chain.accepted_block.connect( [this]( const auto& bsp ){ my->on_block( bsp ); } ));
   my->_irreversible_block_connection.emplace(chain.irreversible_block.connect( [this]( const auto& bsp ){ my->on_irreversible_block( bsp->block ); } ));

   const auto lib_num = chain.last_irreversible_block_num();
   const auto lib = chain.fetch_block_by_number(lib_num);
   if (lib) {
      my->on_irreversible_block(lib);
   } else {
      my->_irreversible_block_time = fc::time_point::maximum();
   }

   if (!my->_producers.empty()) {
      ilog("Launching block production for ${n} producers at ${time}.", ("n", my->_producers.size())("time",fc::time_point::now()));

      if (my->_production_enabled) {
         if (chain.head_block_num() == 0) {
            new_chain_banner(chain);
         }
         //_production_skip_flags |= eosio::chain::skip_undo_history_check;
      }
   }

   my->schedule_production_loop();

   ilog("producer plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void producer_plugin::plugin_shutdown() {
   try {
      my->_timer.cancel();
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
   }

   my->_accepted_block_connection.reset();
   my->_irreversible_block_connection.reset();
}

void producer_plugin::pause() {
   my->_pause_production = true;
}

void producer_plugin::resume() {
   my->_pause_production = false;
   // it is possible that we are only speculating because of this policy which we have now changed
   // re-evaluate that now
   //
   if (my->_pending_block_mode == pending_block_mode::speculating) {
      chain::controller& chain = app().get_plugin<chain_plugin>().chain();
      chain.abort_block();
      my->schedule_production_loop();
   }
}

bool producer_plugin::paused() const {
   return my->_pause_production;
}

void producer_plugin::update_runtime_options(const runtime_options& options) {
   bool check_speculating = false;

   if (options.max_transaction_time) {
      my->_max_transaction_time_ms = *options.max_transaction_time;
   }

   if (options.max_irreversible_block_age) {
      my->_max_irreversible_block_age_us =  fc::seconds(*options.max_irreversible_block_age);
      check_speculating = true;
   }

   if (options.produce_time_offset_us) {
      my->_produce_time_offset_us = *options.produce_time_offset_us;
   }

   if (options.last_block_time_offset_us) {
      my->_last_block_time_offset_us = *options.last_block_time_offset_us;
   }

   if (options.incoming_defer_ratio) {
      my->_incoming_defer_ratio = *options.incoming_defer_ratio;
   }

   if (check_speculating && my->_pending_block_mode == pending_block_mode::speculating) {
      chain::controller& chain = app().get_plugin<chain_plugin>().chain();
      chain.abort_block();
      my->schedule_production_loop();
   }

   if (options.subjective_cpu_leeway_us) {
      chain::controller& chain = app().get_plugin<chain_plugin>().chain();
      chain.set_subjective_cpu_leeway(fc::microseconds(*options.subjective_cpu_leeway_us));
   }
}

producer_plugin::runtime_options producer_plugin::get_runtime_options() const {
   return {
      my->_max_transaction_time_ms,
      my->_max_irreversible_block_age_us.count() < 0 ? -1 : my->_max_irreversible_block_age_us.count() / 1'000'000,
      my->_produce_time_offset_us,
      my->_last_block_time_offset_us
   };
}

void producer_plugin::add_greylist_accounts(const greylist_params& params) {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   for (auto &acc : params.accounts) {
      chain.add_resource_greylist(acc);
   }
}

void producer_plugin::remove_greylist_accounts(const greylist_params& params) {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   for (auto &acc : params.accounts) {
      chain.remove_resource_greylist(acc);
   }
}

producer_plugin::greylist_params producer_plugin::get_greylist() const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   greylist_params result;
   const auto& list = chain.get_resource_greylist();
   result.accounts.reserve(list.size());
   for (auto &acc: list) {
      result.accounts.push_back(acc);
   }
   return result;
}

producer_plugin::whitelist_blacklist producer_plugin::get_whitelist_blacklist() const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   return {
      chain.get_actor_whitelist(),
      chain.get_actor_blacklist(),
      chain.get_contract_whitelist(),
      chain.get_contract_blacklist(),
      chain.get_action_blacklist(),
      chain.get_key_blacklist()
   };
}

void producer_plugin::set_whitelist_blacklist(const producer_plugin::whitelist_blacklist& params) {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   if(params.actor_whitelist.valid()) chain.set_actor_whitelist(*params.actor_whitelist);
   if(params.actor_blacklist.valid()) chain.set_actor_blacklist(*params.actor_blacklist);
   if(params.contract_whitelist.valid()) chain.set_contract_whitelist(*params.contract_whitelist);
   if(params.contract_blacklist.valid()) chain.set_contract_blacklist(*params.contract_blacklist);
   if(params.action_blacklist.valid()) chain.set_action_blacklist(*params.action_blacklist);
   if(params.key_blacklist.valid()) chain.set_key_blacklist(*params.key_blacklist);
}

producer_plugin::integrity_hash_information producer_plugin::get_integrity_hash() const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();

   auto reschedule = fc::make_scoped_exit([this](){
      my->schedule_production_loop();
   });

   if (chain.pending_block_state()) {
      // abort the pending block
      chain.abort_block();
   } else {
      reschedule.cancel();
   }

   return {chain.head_block_id(), chain.calculate_integrity_hash()};
}

producer_plugin::snapshot_information producer_plugin::create_snapshot() const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();

   auto reschedule = fc::make_scoped_exit([this](){
      my->schedule_production_loop();
   });

   if (chain.pending_block_state()) {
      // abort the pending block
      chain.abort_block();
   } else {
      reschedule.cancel();
   }

   auto head_id = chain.head_block_id();
   std::string snapshot_path = (my->_snapshots_dir / fc::format_string("snapshot-${id}.bin", fc::mutable_variant_object()("id", head_id))).generic_string();

   EOS_ASSERT( !fc::is_regular_file(snapshot_path), snapshot_exists_exception,
               "snapshot named ${name} already exists", ("name", snapshot_path));


   auto snap_out = std::ofstream(snapshot_path, (std::ios::out | std::ios::binary));
   auto writer = std::make_shared<ostream_snapshot_writer>(snap_out);
   chain.write_snapshot(writer);
   writer->finalize();
   snap_out.flush();
   snap_out.close();

   return {head_id, snapshot_path};
}

optional<fc::time_point> producer_plugin_impl::calculate_next_block_time(const account_name& producer_name, const block_timestamp_type& current_block_time) const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto& hbs = chain.head_block_state();
   const auto& active_schedule = hbs->active_schedule.producers;

   // determine if this producer is in the active schedule and if so, where
   auto itr = std::find_if(active_schedule.begin(), active_schedule.end(), [&](const auto& asp){ return asp.producer_name == producer_name; });
   if (itr == active_schedule.end()) {
      // this producer is not in the active producer set
      return optional<fc::time_point>();
   }

   size_t producer_index = itr - active_schedule.begin();
   uint32_t minimum_offset = 1; // must at least be the "next" block

   // account for a watermark in the future which is disqualifying this producer for now
   // this is conservative assuming no blocks are dropped.  If blocks are dropped the watermark will
   // disqualify this producer for longer but it is assumed they will wake up, determine that they
   // are disqualified for longer due to skipped blocks and re-caculate their next block with better
   // information then
   auto current_watermark_itr = _producer_watermarks.find(producer_name);
   if (current_watermark_itr != _producer_watermarks.end()) {
      auto watermark = current_watermark_itr->second;
      auto block_num = chain.head_block_state()->block_num;
      if (chain.pending_block_state()) {
         ++block_num;
      }
      if (watermark > block_num) {
         // if I have a watermark then I need to wait until after that watermark
         minimum_offset = watermark - block_num + 1;
      }
   }

   // this producers next opportuity to produce is the next time its slot arrives after or at the calculated minimum
   uint32_t minimum_slot = current_block_time.slot + minimum_offset;
   size_t minimum_slot_producer_index = (minimum_slot % (active_schedule.size() * config::producer_repetitions)) / config::producer_repetitions;
   if ( producer_index == minimum_slot_producer_index ) {
      // this is the producer for the minimum slot, go with that
      return block_timestamp_type(minimum_slot).to_time_point();
   } else {
      // calculate how many rounds are between the minimum producer and the producer in question
      size_t producer_distance = producer_index - minimum_slot_producer_index;
      // check for unsigned underflow
      if (producer_distance > producer_index) {
         producer_distance += active_schedule.size();
      }

      // align the minimum slot to the first of its set of reps
      uint32_t first_minimum_producer_slot = minimum_slot - (minimum_slot % config::producer_repetitions);

      // offset the aligned minimum to the *earliest* next set of slots for this producer
      uint32_t next_block_slot = first_minimum_producer_slot  + (producer_distance * config::producer_repetitions);
      return block_timestamp_type(next_block_slot).to_time_point();
   }
}

fc::time_point producer_plugin_impl::calculate_pending_block_time() const {
   const chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const fc::time_point now = fc::time_point::now();
   const fc::time_point base = std::max<fc::time_point>(now, chain.head_block_time());
   const int64_t min_time_to_next_block = (config::block_interval_us) - (base.time_since_epoch().count() % (config::block_interval_us) );
   fc::time_point block_time = base + fc::microseconds(min_time_to_next_block);


   if((block_time - now) < fc::microseconds(config::block_interval_us/10) ) {     // we must sleep for at least 50ms
      block_time += fc::microseconds(config::block_interval_us);
   }
   return block_time;
}

enum class tx_category {
   PERSISTED,
   UNEXPIRED_UNPERSISTED,
   EXPIRED,
};


producer_plugin_impl::start_block_result producer_plugin_impl::start_block(bool &last_block) {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();

   if( chain.get_read_mode() == chain::db_read_mode::READ_ONLY )
      return start_block_result::waiting;

   const auto& hbs = chain.head_block_state();

   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms (1/10 of block_interval), wait for the whole block interval.
   const fc::time_point now = fc::time_point::now();
   const fc::time_point block_time = calculate_pending_block_time();

   _pending_block_mode = pending_block_mode::producing;

   // Not our turn
   last_block = ((block_timestamp_type(block_time).slot % config::producer_repetitions) == config::producer_repetitions - 1);
   const auto& scheduled_producer = hbs->get_scheduled_producer(block_time);
   auto currrent_watermark_itr = _producer_watermarks.find(scheduled_producer.producer_name);
   auto signature_provider_itr = _signature_providers.find(scheduled_producer.block_signing_key);
   auto irreversible_block_age = get_irreversible_block_age();

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled ) {
      _pending_block_mode = pending_block_mode::speculating;
   } else if( _producers.find(scheduled_producer.producer_name) == _producers.end()) {
      _pending_block_mode = pending_block_mode::speculating;
   } else if (signature_provider_itr == _signature_providers.end()) {
      elog("Not producing block because I don't have the private key for ${scheduled_key}", ("scheduled_key", scheduled_producer.block_signing_key));
      _pending_block_mode = pending_block_mode::speculating;
   } else if ( _pause_production ) {
      elog("Not producing block because production is explicitly paused");
      _pending_block_mode = pending_block_mode::speculating;
   } else if ( _max_irreversible_block_age_us.count() >= 0 && irreversible_block_age >= _max_irreversible_block_age_us ) {
      elog("Not producing block because the irreversible block is too old [age:${age}s, max:${max}s]", ("age", irreversible_block_age.count() / 1'000'000)( "max", _max_irreversible_block_age_us.count() / 1'000'000 ));
      _pending_block_mode = pending_block_mode::speculating;
   }

   if (_pending_block_mode == pending_block_mode::producing) {
      // determine if our watermark excludes us from producing at this point
      if (currrent_watermark_itr != _producer_watermarks.end()) {
         if (currrent_watermark_itr->second >= hbs->block_num + 1) {
            elog("Not producing block because \"${producer}\" signed a BFT confirmation OR block at a higher block number (${watermark}) than the current fork's head (${head_block_num})",
                ("producer", scheduled_producer.producer_name)
                ("watermark", currrent_watermark_itr->second)
                ("head_block_num", hbs->block_num));
            _pending_block_mode = pending_block_mode::speculating;
         }
      }
   }

   if (_pending_block_mode == pending_block_mode::speculating) {
      auto head_block_age = now - chain.head_block_time();
      if (head_block_age > fc::seconds(5))
         return start_block_result::waiting;
   }

   try {
      uint16_t blocks_to_confirm = 0;

      if (_pending_block_mode == pending_block_mode::producing) {
         // determine how many blocks this producer can confirm
         // 1) if it is not a producer from this node, assume no confirmations (we will discard this block anyway)
         // 2) if it is a producer on this node that has never produced, the conservative approach is to assume no
         //    confirmations to make sure we don't double sign after a crash TODO: make these watermarks durable?
         // 3) if it is a producer on this node where this node knows the last block it produced, safely set it -UNLESS-
         // 4) the producer on this node's last watermark is higher (meaning on a different fork)
         if (currrent_watermark_itr != _producer_watermarks.end()) {
            auto watermark = currrent_watermark_itr->second;
            if (watermark < hbs->block_num) {
               blocks_to_confirm = std::min<uint16_t>(std::numeric_limits<uint16_t>::max(), (uint16_t)(hbs->block_num - watermark));
            }
         }
      }

      chain.abort_block();
      chain.start_block(block_time, blocks_to_confirm);
   } FC_LOG_AND_DROP();

   const auto& pbs = chain.pending_block_state();
   if (pbs) {

      if (_pending_block_mode == pending_block_mode::producing && pbs->block_signing_key != scheduled_producer.block_signing_key) {
         elog("Block Signing Key is not expected value, reverting to speculative mode! [expected: \"${expected}\", actual: \"${actual\"", ("expected", scheduled_producer.block_signing_key)("actual", pbs->block_signing_key));
         _pending_block_mode = pending_block_mode::speculating;
      }

      // attempt to play persisted transactions first
      bool exhausted = false;

      // remove all persisted transactions that have now expired
      auto& persisted_by_id = _persistent_transactions.get<by_id>();
      auto& persisted_by_expiry = _persistent_transactions.get<by_expiry>();
      if (!persisted_by_expiry.empty()) {
         int num_expired_persistent = 0;
         int orig_count = _persistent_transactions.size();

         while(!persisted_by_expiry.empty() && persisted_by_expiry.begin()->expiry <= pbs->header.timestamp.to_time_point()) {
            auto const& txid = persisted_by_expiry.begin()->trx_id;
            if (_pending_block_mode == pending_block_mode::producing) {
               fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} is EXPIRING PERSISTED tx: ${txid}",
                       ("block_num", chain.head_block_num() + 1)
                       ("prod", chain.pending_block_state()->header.producer)
                       ("txid", txid));
            } else {
               fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution is EXPIRING PERSISTED tx: ${txid}",
                       ("txid", txid));
            }

            persisted_by_expiry.erase(persisted_by_expiry.begin());
            num_expired_persistent++;
         }

         fc_dlog(_log, "Processed ${n} persisted transactions, Expired ${expired}",
                ("n", orig_count)
                ("expired", num_expired_persistent));
      }

      try {
         size_t orig_pending_txn_size = _pending_incoming_transactions.size();

         // Processing unapplied transactions...
         //
         if (_producers.empty() && persisted_by_id.empty()) {
            // if this node can never produce and has no persisted transactions,
            // there is no need for unapplied transactions they can be dropped
            chain.drop_all_unapplied_transactions();
         } else {
            std::vector<transaction_metadata_ptr> apply_trxs;
            { // derive appliable transactions from unapplied_transactions and drop droppable transactions
               auto unapplied_trxs = chain.get_unapplied_transactions();
               apply_trxs.reserve(unapplied_trxs.size());

               auto calculate_transaction_category = [&](const transaction_metadata_ptr& trx) {
                  if (trx->packed_trx.expiration() < pbs->header.timestamp.to_time_point()) {
                     return tx_category::EXPIRED;
                  } else if (persisted_by_id.find(trx->id) != persisted_by_id.end()) {
                     return tx_category::PERSISTED;
                  } else {
                     return tx_category::UNEXPIRED_UNPERSISTED;
                  }
               };

               for (auto& trx: unapplied_trxs) {
                  auto category = calculate_transaction_category(trx);
                  if (category == tx_category::EXPIRED || (category == tx_category::UNEXPIRED_UNPERSISTED && _producers.empty())) {
                     if (!_producers.empty()) {
                        fc_dlog(_trx_trace_log, "[TRX_TRACE] Node with producers configured is dropping an EXPIRED transaction that was PREVIOUSLY ACCEPTED : ${txid}",
                               ("txid", trx->id));
                     }
                     chain.drop_unapplied_transaction(trx);
                  } else if (category == tx_category::PERSISTED || (category == tx_category::UNEXPIRED_UNPERSISTED && _pending_block_mode == pending_block_mode::producing)) {
                     apply_trxs.emplace_back(std::move(trx));
                  }
               }
            }

            if (!apply_trxs.empty()) {
               int num_applied = 0;
               int num_failed = 0;
               int num_processed = 0;

               for (const auto& trx: apply_trxs) {
                  if (block_time <= fc::time_point::now()) exhausted = true;
                  if (exhausted) {
                     break;
                  }

                  num_processed++;

                  try {
                     auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
                     bool deadline_is_subjective = false;
                     if (_max_transaction_time_ms < 0 || (_pending_block_mode == pending_block_mode::producing && block_time < deadline)) {
                        deadline_is_subjective = true;
                        deadline = block_time;
                     }

                     auto trace = chain.push_transaction(trx, deadline);
                     if (trace->except) {
                        if (failure_is_subjective(*trace->except, deadline_is_subjective)) {
                           exhausted = true;
                        } else {
                           // this failed our configured maximum transaction time, we don't want to replay it
                           chain.drop_unapplied_transaction(trx);
                           num_failed++;
                        }
                     } else {
                        num_applied++;
                     }
                  } catch ( const guard_exception& e ) {
                     app().get_plugin<chain_plugin>().handle_guard_exception(e);
                     return start_block_result::failed;
                  } FC_LOG_AND_DROP();
               }

               fc_dlog(_log, "Processed ${m} of ${n} previously applied transactions, Applied ${applied}, Failed/Dropped ${failed}",
                      ("m", num_processed)
                      ("n", apply_trxs.size())
                      ("applied", num_applied)
                      ("failed", num_failed));
            }
         }

         if (_pending_block_mode == pending_block_mode::producing) {
            auto& blacklist_by_id = _blacklisted_transactions.get<by_id>();
            auto& blacklist_by_expiry = _blacklisted_transactions.get<by_expiry>();
            auto now = fc::time_point::now();
            if(!blacklist_by_expiry.empty()) {
               int num_expired = 0;
               int orig_count = _blacklisted_transactions.size();

               while (!blacklist_by_expiry.empty() && blacklist_by_expiry.begin()->expiry <= now) {
                  blacklist_by_expiry.erase(blacklist_by_expiry.begin());
                  num_expired++;
               }

               fc_dlog(_log, "Processed ${n} blacklisted transactions, Expired ${expired}",
                      ("n", orig_count)
                      ("expired", num_expired));
            }

            auto scheduled_trxs = chain.get_scheduled_transactions();
            if (!scheduled_trxs.empty()) {
               int num_applied = 0;
               int num_failed = 0;
               int num_processed = 0;

               for (const auto& trx : scheduled_trxs) {
                  if (block_time <= fc::time_point::now()) exhausted = true;
                  if (exhausted) {
                     break;
                  }

                  num_processed++;

                  // configurable ratio of incoming txns vs deferred txns
                  while (_incoming_trx_weight >= 1.0 && orig_pending_txn_size && _pending_incoming_transactions.size()) {
                     auto e = _pending_incoming_transactions.front();
                     _pending_incoming_transactions.pop_front();
                     --orig_pending_txn_size;
                     _incoming_trx_weight -= 1.0;
                     on_incoming_transaction_async(std::get<0>(e), std::get<1>(e), std::get<2>(e));
                  }

                  if (block_time <= fc::time_point::now()) {
                     exhausted = true;
                     break;
                  }

                  if (blacklist_by_id.find(trx) != blacklist_by_id.end()) {
                     continue;
                  }

                  try {
                     auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
                     bool deadline_is_subjective = false;
                     if (_max_transaction_time_ms < 0 || (_pending_block_mode == pending_block_mode::producing && block_time < deadline)) {
                        deadline_is_subjective = true;
                        deadline = block_time;
                     }

                     auto trace = chain.push_scheduled_transaction(trx, deadline);
                     if (trace->except) {
                        if (failure_is_subjective(*trace->except, deadline_is_subjective)) {
                           exhausted = true;
                        } else {
                           auto expiration = fc::time_point::now() + fc::seconds(chain.get_global_properties().configuration.deferred_trx_expiration_window);
                           // this failed our configured maximum transaction time, we don't want to replay it add it to a blacklist
                           _blacklisted_transactions.insert(transaction_id_with_expiry{trx, expiration});
                           num_failed++;
                        }
                     } else {
                        num_applied++;
                     }
                  } catch ( const guard_exception& e ) {
                     app().get_plugin<chain_plugin>().handle_guard_exception(e);
                     return start_block_result::failed;
                  } FC_LOG_AND_DROP();

                  _incoming_trx_weight += _incoming_defer_ratio;
                  if (!orig_pending_txn_size) _incoming_trx_weight = 0.0;
               }

               fc_dlog(_log, "Processed ${m} of ${n} scheduled transactions, Applied ${applied}, Failed/Dropped ${failed}",
                      ("m", num_processed)
                      ("n", scheduled_trxs.size())
                      ("applied", num_applied)
                      ("failed", num_failed));

            }
         }

         if (exhausted || block_time <= fc::time_point::now()) {
            return start_block_result::exhausted;
         } else {
            // attempt to apply any pending incoming transactions
            _incoming_trx_weight = 0.0;

            if (!_pending_incoming_transactions.empty()) {
               fc_dlog(_log, "Processing ${n} pending transactions");
               while (orig_pending_txn_size && _pending_incoming_transactions.size()) {
                  auto e = _pending_incoming_transactions.front();
                  _pending_incoming_transactions.pop_front();
                  --orig_pending_txn_size;
                  on_incoming_transaction_async(std::get<0>(e), std::get<1>(e), std::get<2>(e));
                  if (block_time <= fc::time_point::now()) return start_block_result::exhausted;
               }
            }
            return start_block_result::succeeded;
         }

      } catch ( boost::interprocess::bad_alloc& ) {
         chain_plugin::handle_db_exhaustion();
         return start_block_result::failed;
      }

   }

   return start_block_result::failed;
}

void producer_plugin_impl::schedule_production_loop() {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   _timer.cancel();
   std::weak_ptr<producer_plugin_impl> weak_this = shared_from_this();

   bool last_block;
   auto result = start_block(last_block);

   if (result == start_block_result::failed) {
      elog("Failed to start a pending block, will try again later");
      _timer.expires_from_now( boost::posix_time::microseconds( config::block_interval_us  / 10 ));

      // we failed to start a block, so try again later?
      _timer.async_wait([weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
         auto self = weak_this.lock();
         if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
            self->schedule_production_loop();
         }
      });
   } else if (result == start_block_result::waiting){
      if (!_producers.empty() && !production_disabled_by_policy()) {
         fc_dlog(_log, "Waiting till another block is received and scheduling Speculative/Production Change");
         schedule_delayed_production_loop(weak_this, calculate_pending_block_time());
      } else {
         fc_dlog(_log, "Waiting till another block is received");
         // nothing to do until more blocks arrive
      }

   } else if (_pending_block_mode == pending_block_mode::producing) {

      // we succeeded but block may be exhausted
      static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
      if (result == start_block_result::succeeded) {
         // ship this block off no later than its deadline
         EOS_ASSERT( chain.pending_block_state(), missing_pending_block_state, "producing without pending_block_state, start_block succeeded" );
         auto deadline = chain.pending_block_time().time_since_epoch().count() + (last_block ? _last_block_time_offset_us : _produce_time_offset_us);
         _timer.expires_at( epoch + boost::posix_time::microseconds( deadline ));
         fc_dlog(_log, "Scheduling Block Production on Normal Block #${num} for ${time}", ("num", chain.pending_block_state()->block_num)("time",deadline));
      } else {
         EOS_ASSERT( chain.pending_block_state(), missing_pending_block_state, "producing without pending_block_state" );
         auto expect_time = chain.pending_block_time() - fc::microseconds(config::block_interval_us);
         // ship this block off up to 1 block time earlier or immediately
         if (fc::time_point::now() >= expect_time) {
            _timer.expires_from_now( boost::posix_time::microseconds( 0 ));
            fc_dlog(_log, "Scheduling Block Production on Exhausted Block #${num} immediately", ("num", chain.pending_block_state()->block_num));
         } else {
            _timer.expires_at(epoch + boost::posix_time::microseconds(expect_time.time_since_epoch().count()));
            fc_dlog(_log, "Scheduling Block Production on Exhausted Block #${num} at ${time}", ("num", chain.pending_block_state()->block_num)("time",expect_time));
         }
      }

      _timer.async_wait([&chain,weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
         auto self = weak_this.lock();
         if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
            // pending_block_state expected, but can't assert inside async_wait
            auto block_num = chain.pending_block_state() ? chain.pending_block_state()->block_num : 0;
            auto res = self->maybe_produce_block();
            fc_dlog(_log, "Producing Block #${num} returned: ${res}", ("num", block_num)("res", res));
         }
      });
   } else if (_pending_block_mode == pending_block_mode::speculating && !_producers.empty() && !production_disabled_by_policy()){
      fc_dlog(_log, "Specualtive Block Created; Scheduling Speculative/Production Change");
      EOS_ASSERT( chain.pending_block_state(), missing_pending_block_state, "speculating without pending_block_state" );
      const auto& pbs = chain.pending_block_state();
      schedule_delayed_production_loop(weak_this, pbs->header.timestamp);
   } else {
      fc_dlog(_log, "Speculative Block Created");
   }
}

void producer_plugin_impl::schedule_delayed_production_loop(const std::weak_ptr<producer_plugin_impl>& weak_this, const block_timestamp_type& current_block_time) {
   // if we have any producers then we should at least set a timer for our next available slot
   optional<fc::time_point> wake_up_time;
   for (const auto&p: _producers) {
      auto next_producer_block_time = calculate_next_block_time(p, current_block_time);
      if (next_producer_block_time) {
         auto producer_wake_up_time = *next_producer_block_time - fc::microseconds(config::block_interval_us);
         if (wake_up_time) {
            // wake up with a full block interval to the deadline
            wake_up_time = std::min<fc::time_point>(*wake_up_time, producer_wake_up_time);
         } else {
            wake_up_time = producer_wake_up_time;
         }
      }
   }

   if (wake_up_time) {
      fc_dlog(_log, "Scheduling Speculative/Production Change at ${time}", ("time", wake_up_time));
      static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
      _timer.expires_at(epoch + boost::posix_time::microseconds(wake_up_time->time_since_epoch().count()));
      _timer.async_wait([weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
         auto self = weak_this.lock();
         if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
            self->schedule_production_loop();
         }
      });
   } else {
      fc_dlog(_log, "Not Scheduling Speculative/Production, no local producers had valid wake up times");
   }
}


bool producer_plugin_impl::maybe_produce_block() {
   auto reschedule = fc::make_scoped_exit([this]{
      schedule_production_loop();
   });

   try {
      try {
         produce_block();
         return true;
      } catch ( const guard_exception& e ) {
         app().get_plugin<chain_plugin>().handle_guard_exception(e);
         return false;
      } FC_LOG_AND_DROP();
   } catch ( boost::interprocess::bad_alloc&) {
      raise(SIGUSR1);
      return false;
   }

   fc_dlog(_log, "Aborting block due to produce_block error");
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   chain.abort_block();
   return false;
}

static auto make_debug_time_logger() {
   auto start = fc::time_point::now();
   return fc::make_scoped_exit([=](){
      fc_dlog(_log, "Signing took ${ms}us", ("ms", fc::time_point::now() - start) );
   });
}

static auto maybe_make_debug_time_logger() -> fc::optional<decltype(make_debug_time_logger())> {
   if (_log.is_enabled( fc::log_level::debug ) ){
      return make_debug_time_logger();
   } else {
      return {};
   }
}

void producer_plugin_impl::produce_block() {
   //ilog("produce_block ${t}", ("t", fc::time_point::now())); // for testing _produce_time_offset_us
   EOS_ASSERT(_pending_block_mode == pending_block_mode::producing, producer_exception, "called produce_block while not actually producing");
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto& pbs = chain.pending_block_state();
   const auto& hbs = chain.head_block_state();
   EOS_ASSERT(pbs, missing_pending_block_state, "pending_block_state does not exist but it should, another plugin may have corrupted it");
   auto signature_provider_itr = _signature_providers.find( pbs->block_signing_key );

   EOS_ASSERT(signature_provider_itr != _signature_providers.end(), producer_priv_key_not_found, "Attempting to produce a block for which we don't have the private key");

   //idump( (fc::time_point::now() - chain.pending_block_time()) );
   chain.finalize_block();
   chain.sign_block( [&]( const digest_type& d ) {
      auto debug_logger = maybe_make_debug_time_logger();
      return signature_provider_itr->second(d);
   } );

   chain.commit_block();
   auto hbt = chain.head_block_time();
   //idump((fc::time_point::now() - hbt));

   block_state_ptr new_bs = chain.head_block_state();
   _producer_watermarks[new_bs->header.producer] = chain.head_block_num();

   ilog("Produced block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, confirmed: ${confs}]",
        ("p",new_bs->header.producer)("id",fc::variant(new_bs->id).as_string().substr(0,16))
        ("n",new_bs->block_num)("t",new_bs->header.timestamp)
        ("count",new_bs->block->transactions.size())("lib",chain.last_irreversible_block_num())("confs", new_bs->header.confirmed));

}

} // namespace eosio
