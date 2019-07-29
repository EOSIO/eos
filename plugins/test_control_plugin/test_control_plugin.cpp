/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/test_control_plugin/test_control_plugin.hpp>
#include <fc/optional.hpp>
#include <atomic>

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _test_control_plugin = app().register_plugin<test_control_plugin>();

class test_control_plugin_impl {
public:
   test_control_plugin_impl(chain::controller& c) : _chain(c) {}
   void connect();
   void disconnect();
   void kill_on_lib(account_name prod, uint32_t where_in_seq);
   void kill_on_head(account_name prod, uint32_t where_in_seq);

private:
   void accepted_block(const chain::block_state_ptr& bsp);
   void applied_irreversible_block(const chain::block_state_ptr& bsp);
   void process_next_block_state(const chain::block_state_ptr& bsp);

   fc::optional<boost::signals2::scoped_connection> _accepted_block_connection;
   fc::optional<boost::signals2::scoped_connection> _irreversible_block_connection;
   chain::controller&  _chain;
   account_name        _producer;
   int32_t             _where_in_sequence;
   int32_t             _producer_sequence;
   bool                _clean_producer_sequence;
   std::atomic_bool    _track_lib;
   std::atomic_bool    _track_head;
};

void test_control_plugin_impl::connect() {
   _irreversible_block_connection.emplace(
         _chain.irreversible_block.connect( [&]( const chain::block_state_ptr& bs ) {
            applied_irreversible_block( bs );
         } ));
   _accepted_block_connection =
         _chain.accepted_block.connect( [&]( const chain::block_state_ptr& bs ) {
            accepted_block( bs );
         } );
}

void test_control_plugin_impl::disconnect() {
   _accepted_block_connection.reset();
   _irreversible_block_connection.reset();
}

void test_control_plugin_impl::applied_irreversible_block(const chain::block_state_ptr& bsp) {
   if (_track_lib)
      process_next_block_state(bsp);
}

void test_control_plugin_impl::accepted_block(const chain::block_state_ptr& bsp) {
   if (_track_head)
      process_next_block_state(bsp);
}

void test_control_plugin_impl::process_next_block_state(const chain::block_state_ptr& bsp) {
   const auto block_time = _chain.head_block_time() + fc::microseconds(chain::config::block_interval_us);
   const auto& producer_authority = bsp->get_scheduled_producer(block_time);
   const auto producer_name = producer_authority.producer_name;
   if (_producer != account_name())
      ilog("producer ${cprod}, looking for ${lprod}", ("cprod", producer_name.to_string())("lprod", _producer.to_string()));

   // start counting sequences for this producer (once we have a sequence that we saw the initial block for that producer)
   if (producer_name == _producer && _clean_producer_sequence) {
      ilog("producer ${prod} seq: ${seq}", ("prod", producer_name.to_string())("seq", _producer_sequence));
      _producer_sequence += 1;

      if (_producer_sequence >= _where_in_sequence) {
         ilog("shutting down");
         app().quit();
      }
   } else if (producer_name != _producer) {
      if (_producer_sequence != -1)
         ilog("producer changed, restarting");
      _producer_sequence = -1;
      // can now guarantee we are at the start of the producer
      _clean_producer_sequence = true;
   }
}

void test_control_plugin_impl::kill_on_lib(account_name prod, uint32_t where_in_seq) {
   _track_head = false;
   _producer = prod;
   _where_in_sequence = static_cast<uint32_t>(where_in_seq);
   _producer_sequence = -1;
   _clean_producer_sequence = false;
   _track_lib = true;
}

void test_control_plugin_impl::kill_on_head(account_name prod, uint32_t where_in_seq) {
   _track_lib = false;
   _producer = prod;
   _where_in_sequence = static_cast<uint32_t>(where_in_seq);
   _producer_sequence = -1;
   _clean_producer_sequence = false;
   _track_head = true;
}

test_control_plugin::test_control_plugin()
{
}

void test_control_plugin::set_program_options(options_description& cli, options_description& cfg) {
}

void test_control_plugin::plugin_initialize(const variables_map& options) {
}

void test_control_plugin::plugin_startup() {
   ilog("test_control_plugin starting up");
   my.reset(new test_control_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   my->connect();
}

void test_control_plugin::plugin_shutdown() {
   my->disconnect();
   ilog("test_control_plugin shutting down");
}

namespace test_control_apis {
read_write::kill_node_on_producer_results read_write::kill_node_on_producer(const read_write::kill_node_on_producer_params& params) const {

   if (params.based_on_lib) {
      ilog("kill on lib for producer: ${p} at their ${s} slot in sequence", ("p", params.producer.to_string())("s", params.where_in_sequence));
      my->kill_on_lib(params.producer, params.where_in_sequence);
   } else {
      ilog("kill on head for producer: ${p} at their ${s} slot in sequence", ("p", params.producer.to_string())("s", params.where_in_sequence));
      my->kill_on_head(params.producer, params.where_in_sequence);
   }
   return read_write::kill_node_on_producer_results{};
}

} // namespace test_control_apis

} // namespace eosio
