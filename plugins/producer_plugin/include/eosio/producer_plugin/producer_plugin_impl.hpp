#pragma once

#include <appbase/application.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio/io_service.hpp>

#include <eosio/chain/block_state.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/unapplied_transaction_queue.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_client_plugin/http_client_plugin.hpp>

#include <fc/network/url.hpp>
#include <fc/time.hpp>

#include <eosio/producer_plugin/common.hpp>
#include <eosio/producer_plugin/pending_snapshot.hpp>

#include <memory>
#include <optional>




namespace eosio {

class producer_plugin_impl : public std::enable_shared_from_this<producer_plugin_impl> {
   public:
      using producer_watermark = std::pair<uint32_t, eosio::chain::block_timestamp_type>;
      using signature_provider_type = std::function<chain::signature_type(chain::digest_type)>;
      
      enum class start_block_result {
         succeeded,
         failed,
         waiting_for_block,
         waiting_for_production,
         exhausted
      };
      
      producer_plugin_impl(boost::asio::io_service& io);

      void consider_new_watermark( eosio::chain::account_name producer, uint32_t block_num, eosio::chain::block_timestamp_type timestamp );
      std::optional<producer_watermark> get_watermark( eosio::chain::account_name producer ) const;
      void on_block( const eosio::chain::block_state_ptr& bsp );
      void on_block_header( const eosio::chain::block_state_ptr& bsp );
      void on_irreversible_block( const eosio::chain::signed_block_ptr& lib );
      template<typename Type, typename Channel, typename F>
      auto publish_results_of(const Type &data, Channel& channel, F f);
      fc::time_point calculate_pending_block_time() const;
      fc::microseconds get_irreversible_block_age();
      start_block_result start_block();
      // Example:
      // --> Start block A (block time x.500) at time x.000
      // -> start_block()
      // --> deadline, produce block x.500 at time x.400 (assuming 80% cpu block effort)
      // -> Idle
      // --> Start block B (block time y.000) at time x.500
      void schedule_production_loop();
      bool on_incoming_block(const eosio::chain::signed_block_ptr& block, const std::optional<eosio::chain::block_id_type>& block_id);
      std::optional<fc::time_point> calculate_next_block_time(const eosio::chain::account_name& producer_name, const eosio::chain::block_timestamp_type& current_block_time) const;
      void schedule_maybe_produce_block( bool exhausted );
      void produce_block();
      bool maybe_produce_block();
      bool remove_expired_trxs( const fc::time_point& deadline );
      bool block_is_exhausted() const;
      bool remove_expired_blacklisted_trxs( const fc::time_point& deadline );
      bool process_unapplied_trxs( const fc::time_point& deadline );
      void process_scheduled_and_incoming_trxs( const fc::time_point& deadline, size_t& pending_incoming_process_limit );
      bool process_incoming_trxs( const fc::time_point& deadline, size_t& pending_incoming_process_limit );
      void on_incoming_transaction_async(const eosio::chain::packed_transaction_ptr& trx, bool persist_until_expired, eosio::producer_plugin::next_function<eosio::chain::transaction_trace_ptr> next);
      bool process_incoming_transaction_async(const eosio::chain::transaction_metadata_ptr& trx, bool persist_until_expired, eosio::producer_plugin::next_function<eosio::chain::transaction_trace_ptr> next);
      bool production_disabled_by_policy();
      fc::time_point calculate_block_deadline( const fc::time_point& ) const;
      void schedule_delayed_production_loop(const std::weak_ptr<producer_plugin_impl>& weak_this, optional<fc::time_point> wake_up_time);
      optional<fc::time_point> calculate_producer_wake_up_time( const eosio::chain::block_timestamp_type& ref_block_time ) const;

      boost::asio::deadline_timer                               _timer;
      eosio::chain::plugin_interface::compat::channels::transaction_ack::channel_type&        _transaction_ack_channel;
      std::map<eosio::chain::account_name, producer_plugin_impl::producer_watermark>         _producer_watermarks;
      std::set<eosio::chain::account_name>                             _producers;
      eosio::chain::unapplied_transaction_queue                               _unapplied_transactions;
      fc::time_point                                            _irreversible_block_time;
      eosio::chain_plugin*                                             chain_plug = nullptr;
      eosio::pending_snapshot_index                                   _pending_snapshot_index;
      pending_block_mode                                        _pending_block_mode = eosio::pending_block_mode::speculating;
      std::map<eosio::chain::public_key_type, signature_provider_type> _signature_providers;
      bool     _production_enabled                 = false;
      bool     _pause_production                   = false;
      fc::microseconds                                          _max_irreversible_block_age_us;
      boost::program_options::variables_map _options;
      fc::optional<eosio::chain::named_thread_pool>                           _thread_pool;
      std::atomic<int32_t>                                      _max_transaction_time_ms; // modified by app thread, read by net_plugin thread pool
      int32_t                                                   _produce_time_offset_us = 0;
      int32_t                                                   _last_block_time_offset_us = 0;
      uint32_t                                                  _max_block_cpu_usage_threshold_us = 0;
      uint32_t                                                  _max_block_net_usage_threshold_bytes = 0;
      int32_t                                                   _max_scheduled_transaction_time_per_block_ms = 0;
      fc::microseconds                                          _keosd_provider_timeout_us;
      std::vector<eosio::chain::digest_type>                           _protocol_features_to_activate;
      bool                                                      _protocol_features_signaled = false; // to mark whether it has been signaled in start_block
      chain::plugin_interface::incoming::channels::block::channel_type::handle         _incoming_block_subscription;
      chain::plugin_interface::incoming::channels::transaction::channel_type::handle   _incoming_transaction_subscription;
      chain::plugin_interface::incoming::methods::block_sync::method_type::handle        _incoming_block_sync_provider;
      chain::plugin_interface::incoming::methods::transaction_async::method_type::handle _incoming_transaction_async_provider;
      transaction_id_with_expiry_index                         _blacklisted_transactions;
      fc::optional<boost::signals2::scoped_connection>                          _accepted_block_connection;
      fc::optional<boost::signals2::scoped_connection>                          _accepted_block_header_connection;
      fc::optional<boost::signals2::scoped_connection>                          _irreversible_block_connection;
      
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
      double _incoming_defer_ratio = 1.0; // 1:1
      
      // path to write the snapshots to
      bfs::path _snapshots_dir;
};

static eosio::producer_plugin_impl::signature_provider_type make_key_signature_provider(const eosio::chain::private_key_type& key) {
   return [key]( const chain::digest_type& digest ) {
      return key.sign(digest);
   };
}

static eosio::producer_plugin_impl::signature_provider_type make_keosd_signature_provider(const std::shared_ptr<producer_plugin_impl>& impl, const string& url_str, const eosio::chain::public_key_type pubkey) {
   fc::url keosd_url;
   if(boost::algorithm::starts_with(url_str, "unix://"))
      //send the entire string after unix:// to http_plugin. It'll auto-detect which part
      // is the unix socket path, and which part is the url to hit on the server
      keosd_url = fc::url("unix", url_str.substr(7), fc::ostring(), fc::ostring(), fc::ostring(), fc::ostring(), fc::ovariant_object(), fc::optional<uint16_t>());
   else
      keosd_url = fc::url(url_str);
   std::weak_ptr<producer_plugin_impl> weak_impl = impl;

   return [weak_impl, keosd_url, pubkey]( const eosio::chain::digest_type& digest ) {
      auto impl = weak_impl.lock();
      if (impl) {
         fc::variant params;
         fc::to_variant(std::make_pair(digest, pubkey), params);
         auto deadline = impl->_keosd_provider_timeout_us.count() >= 0 ? fc::time_point::now() + impl->_keosd_provider_timeout_us : fc::time_point::maximum();
         return app().get_plugin<eosio::http_client_plugin>().get_client().post_sync(keosd_url, params, deadline).as<eosio::chain::signature_type>();
      } else {
         return eosio::chain::signature_type();
      }
   };
}

}
