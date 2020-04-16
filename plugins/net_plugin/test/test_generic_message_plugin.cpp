#include <eosio/net_plugin/test_generic_message_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <fc/optional.hpp>
#include <atomic>

namespace fc { class variant; }

namespace {
   template<typename T>
   void verify_payload(std::size_t payload_size, std::size_t type_size) {
      EOS_ASSERT( payload_size == type_size, eosio::chain::plugin_exception,
                  "generic_message payload size (${ps}) not sized correctly to deserialize a ${name} which requires size: ${ts}",
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
}

namespace eosio {
namespace net {
   using type1 = eosio::test_generic_message::test_generic_message_apis::read_write::type1;
   using type2 = eosio::test_generic_message::test_generic_message_apis::read_write::type2;
   using type3 = eosio::test_generic_message::test_generic_message_apis::read_write::type3;

   template<>
   void convert(const eosio::bytes& payload, type1& t) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      const std::size_t f3_size = sizeof(t.f3);
      verify_payload<type1>(payload.size(), f1_size + f2_size + f3_size);
      std::memcpy((void*)&t.f1, (const void*)payload.data(), f1_size);
      std::memcpy((void*)&t.f2, (const void*)(payload.data() + f1_size), f2_size);
      std::memcpy((void*)&t.f3, (const void*)(payload.data() + f1_size + f2_size), f3_size);
   }

   template<>
   void convert(const type1& t, eosio::bytes& payload) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      const std::size_t f3_size = sizeof(t.f3);
      payload.resize(f1_size + f2_size + f3_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
      std::memcpy((void*)(payload.data() + f1_size), (const void*)&t.f2, f2_size);
      std::memcpy((void*)(payload.data() + f1_size + f2_size), (const void*)&t.f3, f3_size);
   }

   template<>
   void convert(const eosio::bytes& payload, type2& t) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      const std::size_t f3_size = sizeof(t.f3);
      const std::size_t f4_size = sizeof(t.f4);
      verify_payload<type2>(payload.size(), f1_size + f2_size + f3_size + f4_size);
      std::memcpy((void*)&t.f1, (const void*)payload.data(), f1_size);
      std::memcpy((void*)&t.f2, (const void*)(payload.data() + f1_size), f2_size);
      std::memcpy((void*)&t.f3, (const void*)(payload.data() + f1_size + f2_size), f3_size);
      std::memcpy((void*)&t.f4, (const void*)(payload.data() + f1_size + f2_size + f3_size), f4_size);
   }

   template<>
   void convert(const type2& t, eosio::bytes& payload) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      const std::size_t f3_size = sizeof(t.f3);
      const std::size_t f4_size = sizeof(t.f4);
      payload.resize(f1_size + f2_size + f3_size + f4_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
      std::memcpy((void*)(payload.data() + f1_size), (const void*)&t.f2, f2_size);
      std::memcpy((void*)(payload.data() + f1_size + f2_size), (const void*)&t.f3, f3_size);
      std::memcpy((void*)(payload.data() + f1_size + f2_size + f3_size), (const void*)&t.f4, f4_size);
   }

   template<>
   void convert(const eosio::bytes& payload, type3& t) {
      const std::size_t f1_size = sizeof(t.f1);
      verify_payload<type3>(payload.size(), f1_size);
      std::memcpy((void*)&t.f1, (const void*)payload.data(), f1_size);
   }

   template<>
   void convert(const type3& t, eosio::bytes& payload) {
      const std::size_t f1_size = sizeof(t.f1);
      payload.resize(f1_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
   }
}

namespace test_generic_message {

static appbase::abstract_plugin& _test_generic_message_plugin = app().register_plugin<test_generic_message_plugin>();

struct test_generic_message_plugin_impl {
   test_generic_message_plugin_impl(net_plugin& n) : _net_plugin(n) {}
   void register_types();

   net_plugin&    _net_plugin;
   std::vector<std::string> _types;
   received_data  _received_data;
};

test_generic_message_plugin::test_generic_message_plugin()
{
}

void test_generic_message_plugin_impl::register_types()
{
   for (auto type : _types) {
      if (type == "type1") {
         _net_plugin.register_msg<net::type1>([this](const net::type1& t, const string& endpoint) {
            store(t, endpoint, _received_data);
         });
      }
      else if (type == "type2") {
         _net_plugin.register_msg<net::type2>([this](const net::type2& t, const string& endpoint) {
            store(t, endpoint, _received_data);
         });
      }
      else if (type == "type3") {
         _net_plugin.register_msg<net::type3>([this](const net::type3& t, const string& endpoint) {
            store(t, endpoint, _received_data);
         });
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
   const auto types = my->_net_plugin.supported_types(params.include_no_types);
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
   return { .messages = my->_received_data };
}
} // namespace test_generic_message_apis

} } // namespace eosio
