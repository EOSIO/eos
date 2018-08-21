/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
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
   void retrieve_next_block_state(const chain::block_state_ptr& bsp);
   void process_next_block_state(const chain::block_header_state& bhs);

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
   wlog("test_control_plugin::kill_node_on_producer() lib");
   _irreversible_block_connection.emplace(
         _chain.irreversible_block.connect( [&]( const chain::block_state_ptr& bs ) {
            applied_irreversible_block( bs );
         } ));
   wlog("test_control_plugin::kill_node_on_producer() head");
   _accepted_block_connection =
         _chain.accepted_block.connect( [&]( const chain::block_state_ptr& bs ) {
            accepted_block( bs );
         } );
   wlog("test_control_plugin::kill_node_on_producer() head connection created");
}

void test_control_plugin_impl::disconnect() {
   _accepted_block_connection.reset();
   _irreversible_block_connection.reset();
}

void test_control_plugin_impl::applied_irreversible_block(const chain::block_state_ptr& bsp) {
   wlog("test_control_plugin_impl::applied_irreversible_block()");
   if (_track_lib)
      retrieve_next_block_state(bsp);
}

void test_control_plugin_impl::accepted_block(const chain::block_state_ptr& bsp) {
   wlog("test_control_plugin_impl::accepted_block()");
   if (_track_head)
      retrieve_next_block_state(bsp);
}

void test_control_plugin_impl::retrieve_next_block_state(const chain::block_state_ptr& bsp) {
   const auto hbn = bsp->block_num;
   auto new_block_header = bsp->header;
   new_block_header.timestamp = new_block_header.timestamp.next();
   new_block_header.previous = bsp->id;
   auto new_bs = bsp->generate_next(new_block_header.timestamp);
   process_next_block_state(new_bs);
}

void test_control_plugin_impl::process_next_block_state(const chain::block_header_state& bhs) {
   const auto block_time = _chain.head_block_time() + fc::microseconds(chain::config::block_interval_us);
   const auto& producer_name = bhs.get_scheduled_producer(block_time).producer_name;
   // start counting sequences for this producer (once we
   if (producer_name == _producer && _clean_producer_sequence) {
      _producer_sequence += 1;
      wlog("test_control_plugin_impl::process_next_block_state() seq=${seq}",("seq",_producer_sequence));

      if (_producer_sequence >= _where_in_sequence) {
         app().quit();
      }
   } else if (producer_name != _producer) {
      wlog("test_control_plugin_impl::process_next_block_state() reset");
      _producer_sequence = -1;
      // can now guarantee we are at the start of the producer
      _clean_producer_sequence = true;
   }
}

void test_control_plugin_impl::kill_on_lib(account_name prod, uint32_t where_in_seq) {
   wlog("test_control_plugin_impl::kill_on_lib() 1");
   _track_head = false;
   wlog("test_control_plugin_impl::kill_on_lib() 2");
   _producer = prod;
   _where_in_sequence = static_cast<uint32_t>(where_in_seq);
   _producer_sequence = -1;
   _clean_producer_sequence = false;
   wlog("test_control_plugin_impl::kill_on_lib() 3");
   _track_lib = true;
   wlog("test_control_plugin_impl::kill_on_lib() 4");
}

void test_control_plugin_impl::kill_on_head(account_name prod, uint32_t where_in_seq) {
   wlog("test_control_plugin_impl::kill_on_head() 1");
   _track_lib = false;
   wlog("test_control_plugin_impl::kill_on_head() 2");
   _producer = prod;
   _where_in_sequence = static_cast<uint32_t>(where_in_seq);
   _producer_sequence = -1;
   _clean_producer_sequence = false;
   wlog("test_control_plugin_impl::kill_on_head() 3");
   _track_head = true;
   wlog("test_control_plugin_impl::kill_on_head() 4");
}

test_control_plugin::test_control_plugin()
: my(new test_control_plugin_impl(app().get_plugin<chain_plugin>().chain()))
{
}

void test_control_plugin::set_program_options(options_description& cli, options_description& cfg) {
   wlog("test_control_plugin::set_program_options()");
}

void test_control_plugin::plugin_initialize(const variables_map& options) {
   wlog("test_control_plugin::plugin_initialize()");
}

void test_control_plugin::plugin_startup() {
   wlog("test_control_plugin::plugin_startup()");
   my->connect();
}

void test_control_plugin::plugin_shutdown() {
   wlog("test_control_plugin::plugin_shutdown()");
   my->disconnect();
}

namespace test_control_apis {
read_write::kill_node_on_producer_results read_write::kill_node_on_producer(const read_write::kill_node_on_producer_params& params) const {
   wlog("test_control_plugin::kill_node_on_producer() ${prod} ${where_in_seq}  ${head_lib}",("prod",params.producer.to_string())("where_in_seq",params.where_in_sequence)("head_lib", (params.based_on_lib ? "LIB" : "HEAD")));

   if (params.based_on_lib) {
      wlog("test_control_plugin::kill_node_on_producer() kill_on_lib");
      my->kill_on_lib(params.producer, params.where_in_sequence);
      wlog("test_control_plugin::kill_node_on_producer() kill_on_lib done");
   } else {
      wlog("test_control_plugin::kill_node_on_producer() kill_on_head");
      my->kill_on_head(params.producer, params.where_in_sequence);
      wlog("test_control_plugin::kill_node_on_producer() kill_on_head done");
   }
   return read_write::kill_node_on_producer_results{};
}

} // namespace test_control_apis

} // namespace eosio
