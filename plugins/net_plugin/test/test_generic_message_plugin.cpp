#include <eosio/net_plugin/test_generic_message_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <fc/optional.hpp>
#include <atomic>

namespace fc { class variant; }

namespace {
   template<typename T>
   void verify_payload(std::size_t payload_size, std::size_t type_size) {
      EOS_ASSERT( payload_size >= type_size, eosio::chain::plugin_exception,
                  "generic_message payload size (${ps}) not sized correctly to deserialize a ${name} which requires at least a size of: ${ts}",
                  ("ps", payload_size)("name",typeid(T).name())("ts", type_size));
   }

   using received_data = std::vector<eosio::test_generic_message::test_generic_message_apis::read_write::message>;
   template<typename T>
   void store(const T& obj, const std::string& endpoint, received_data& rcvd) {
      fc::variant pretty_output;
      const fc::microseconds ten_seconds { 10 * 1000 * 1000 };
      eosio::abi_serializer::to_variant( obj, pretty_output,
         []( eosio::account_name  ){ return fc::optional<eosio::abi_serializer>(); },
         eosio::abi_serializer::create_yield_function( ten_seconds ) );
      rcvd.push_back({ .endpoint = endpoint, .data = std::move(pretty_output) });
   }

   template<typename T>
   const char* get_type(const T& t) {
      return "<UNKNOWN>";
   }

   const char* get_type(const std::string& t) {
      return "<STRING>";
   }

   const char* get_type(const uint64_t& t) {
      return "<uint64_t>";
   }

   const char* get_type(const bool& t) {
      return "<bool>";
   }

   template<typename T>
   std::size_t size(const T& t) {
      return sizeof(t);
   }

   std::size_t size(const std::string& t) {
      return sizeof(std::size_t) + t.length();
   }

   std::size_t total_size() {
      return 0;
   }

   template<typename T, typename... Types>
   std::size_t total_size(const T& t, Types... vars) {
      const auto s = size(t);
      return total_size(vars...) + s;
   }

   template<typename T>
   void pack(char*& buffer, const T& t) {
      const auto s = sizeof(t);
      std::memcpy((void*)buffer, (const void*)&t, s);
      buffer += s;
   }

   void pack(char*& buffer, const std::string& t) {
      const std::size_t length = t.length();
      const auto s = sizeof(length);
      std::memcpy((void*)buffer, (const void*)&length, s);
      buffer += s;
      std::memcpy((void*)buffer, (const void*)t.data(), length);
      buffer += t.length();
   }

   template<typename T>
   void unpack(const char*& buffer, T& t) {
      const auto s = sizeof(t);
      std::memcpy((void*)&t, buffer, s);
      buffer += s;
   }

   void unpack(const char*& buffer, std::string& t) {
      std::size_t length = 0;
      const auto s = sizeof(length);
      std::memcpy((void*)&length, (const void*)buffer, s);
      buffer += s;
      t.resize(length);
      std::memcpy((void*)t.data(), (const void*)buffer, length);
      buffer += length;
   }

}

namespace eosio {
namespace net {
   using type1 = eosio::test_generic_message::test_generic_message_apis::read_write::type1;
   using type2 = eosio::test_generic_message::test_generic_message_apis::read_write::type2;
   using type3 = eosio::test_generic_message::test_generic_message_apis::read_write::type3;

   template<>
   void convert(const eosio::bytes& payload, type1& t) {
      verify_payload<type1>(payload.size(), total_size(t.f1, t.f2, t.f3));
      const char* payload_offset = payload.data();
      ::unpack(payload_offset, t.f1);
      ::unpack(payload_offset, t.f2);
      ::unpack(payload_offset, t.f3);
   }

   template<>
   void convert(const type1& t, eosio::bytes& payload) {
      payload.resize(total_size(t.f1, t.f2, t.f3));
      char* payload_offset = payload.data();
      ::pack(payload_offset, t.f1);
      ::pack(payload_offset, t.f2);
      ::pack(payload_offset, t.f3);
   }

   template<>
   void convert(const eosio::bytes& payload, type2& t) {
      verify_payload<type2>(payload.size(), total_size(t.f1, t.f2, t.f3, t.f4));
      const char* payload_offset = payload.data();
      ::unpack(payload_offset, t.f1);
      ::unpack(payload_offset, t.f2);
      ::unpack(payload_offset, t.f3);
      ::unpack(payload_offset, t.f4);
   }

   template<>
   void convert(const type2& t, eosio::bytes& payload) {
      payload.resize(total_size(t.f1, t.f2, t.f3, t.f4));
      char* payload_offset = payload.data();
      ::pack(payload_offset, t.f1);
      ::pack(payload_offset, t.f2);
      ::pack(payload_offset, t.f3);
      ::pack(payload_offset, t.f4);
   }

   template<>
   void convert(const eosio::bytes& payload, type3& t) {
      verify_payload<type3>(payload.size(), total_size(t.f1));
      const char* payload_offset = payload.data();
      ::unpack(payload_offset, t.f1);
   }

   template<>
   void convert(const type3& t, eosio::bytes& payload) {
      payload.resize(total_size(t.f1));
      char* payload_offset = payload.data();
      ::pack(payload_offset, t.f1);
   }
}

namespace test_generic_message {

static appbase::abstract_plugin& _test_generic_message_plugin = app().register_plugin<test_generic_message_plugin>();

struct test_generic_message_plugin_impl {
   test_generic_message_plugin_impl(net_plugin& n) : _net_plugin(n) {}
   void register_types();

   net_plugin&    _net_plugin;
   std::vector<std::string> _types;
   std::vector<net::scoped_connection> _signal_connections;
   received_data  _received_data;
   mutable std::mutex _received_data_mtx;
};

test_generic_message_plugin::test_generic_message_plugin()
{
}

void test_generic_message_plugin_impl::register_types()
{
   for (auto type : _types) {
      if (type == "type1") {
         _signal_connections.emplace_back(
            _net_plugin.register_msg<net::type1>([this](const net::type1& t, const string& endpoint) {
               std::unique_lock<std::mutex> g( _received_data_mtx );
               store(t, endpoint, _received_data);
            }));
      }
      else if (type == "type2") {
         _signal_connections.emplace_back(
            _net_plugin.register_msg<net::type2>([this](const net::type2& t, const string& endpoint) {
               std::unique_lock<std::mutex> g( _received_data_mtx );
               store(t, endpoint, _received_data);
            }));
      }
      else if (type == "type3") {
         _signal_connections.emplace_back(
            _net_plugin.register_msg<net::type3>([this](const net::type3& t, const string& endpoint) {
               std::unique_lock<std::mutex> g( _received_data_mtx );
               store(t, endpoint, _received_data);
            }));
      }
   }
}

void test_generic_message_plugin::set_program_options(options_description& cli, options_description& cfg) {
   cfg.add_options()
      ("register-generic-type", boost::program_options::value<std::vector<string>>()->composing()->multitoken(), "Register the given type as a generic_message this node processes.")
      ;
}

void test_generic_message_plugin::plugin_initialize(const variables_map& options) {
   ilog("test_generic_message_plugin initialize");
   my = std::make_shared<test_generic_message_plugin_impl>(app().get_plugin<net_plugin>());

   if( options.count( "register-generic-type" )) {
      my->_types = options["register-generic-type"].as<std::vector<std::string>>();
   }
}

void test_generic_message_plugin::plugin_startup() {
   ilog("test_generic_message_plugin starting up");
   my->register_types();
}

void test_generic_message_plugin::plugin_shutdown() {
   ilog("test_generic_message_plugin shutting down");
   my->_signal_connections.clear();
}

namespace test_generic_message_apis {

read_write::send_type1_results read_write::send_type1(const read_write::send_type1_params& params) const {
   my->_net_plugin.send_generic_message(params.data, params.endpoints);
   return read_write::send_type1_results{};
}

read_write::send_type2_results read_write::send_type2(const read_write::send_type2_params& params) const {
   my->_net_plugin.send_generic_message(params.data, params.endpoints);
   return read_write::send_type2_results{};
}

read_write::send_type3_results read_write::send_type3(const read_write::send_type3_params& params) const {
   my->_net_plugin.send_generic_message(params.data, params.endpoints);
   return read_write::send_type3_results{};
}

read_write::registered_types_results read_write::registered_types( const registered_types_params& params ) const {
   const auto types = my->_net_plugin.supported_types(params.ignore_no_support);
   registered_types_results results;
   for (const auto endpoint_types : types) {
      vector<string> type_names;
      for ( const auto type : endpoint_types.second ) {
         string name;
         if ( typeid(net::type1).hash_code() == type ) {
            name = typeid(net::type1).name();
         }
         else if ( typeid(net::type2).hash_code() == type ) {
            name = typeid(net::type2).name();
         }
         else if ( typeid(net::type3).hash_code() == type ) {
            name = typeid(net::type3).name();
         }
         else {
            name = "unknown-type:" + std::to_string(type);
         }
         type_names.push_back(std::move(name));
      }
      node n { .endpoint = endpoint_types.first, .types = type_names };
      results.nodes.push_back(n);
   }
   return results;
}

read_write::received_data_results read_write::received_data( const received_data_params& params ) const {
   std::unique_lock<std::mutex> g( my->_received_data_mtx );
   if (params.endpoints.empty()) {
      return { .messages = my->_received_data };
   }
   read_write::received_data_results results;
   for (const auto msg : my->_received_data) {
      const auto found = std::find(params.endpoints.begin(), params.endpoints.end(), msg.endpoint);
      if (found != params.endpoints.end()) {
         results.messages.push_back(msg);
      }
   }
   return results;
}
} // namespace test_generic_message_apis

} } // namespace eosio
