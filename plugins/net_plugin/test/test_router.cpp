#define BOOST_TEST_MODULE trace_trace_file
#include <boost/test/included/unit_test.hpp>
#include <fc/io/cfile.hpp>
#include <eosio/net_plugin/generic_message_handler.hpp>
#include <boost/filesystem.hpp>

using namespace eosio;
using namespace eosio::net;

struct test_type1 {
   uint32_t f1 { 0 };
   uint64_t f2 { 0 };
};

struct test_type2 {
   uint64_t f1 { 0 };
   uint32_t f2 { 0 };
   uint8_t f3 { 0 };
};

struct test_type3 {
   uint32_t f1 { 0 };
};

namespace {

   void pack(bytes& payload, const test_type1& t) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      payload.resize(f1_size + f2_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
      std::memcpy((void*)(payload.data() + f1_size), (const void*)&t.f2, f2_size);
   }

   void pack(bytes& payload, const test_type2& t) {
      const std::size_t f1_size = sizeof(t.f1);
      const std::size_t f2_size = sizeof(t.f2);
      const std::size_t f3_size = sizeof(t.f3);
      payload.resize(f1_size + f2_size + f3_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
      std::memcpy((void*)(payload.data() + f1_size), (const void*)&t.f2, f2_size);
      std::memcpy((void*)(payload.data() + f1_size + f2_size), (const void*)&t.f3, f3_size);
   }

   void pack(bytes& payload, const test_type3& t) {
      const std::size_t f1_size = sizeof(t.f1);
      payload.resize(f1_size);
      std::memcpy((void*)payload.data(), (const void*)&t.f1, f1_size);
   }

   template<typename T>
   void pack_generic_message(generic_message& msg, const T& t) {
      msg.type = typeid(T).hash_code();
      pack(msg.payload, t);
   }
}

namespace eosio {
   namespace net {
      template<>
      void convert(const bytes& payload, test_type1& t) {
         const std::size_t f1_size = sizeof(t.f1);
         const std::size_t f2_size = sizeof(t.f2);
         BOOST_REQUIRE_EQUAL(payload.size(), f1_size + f2_size);
         std::memcpy((void*)&t.f1, (const void*)payload.data(), f1_size);
         std::memcpy((void*)&t.f2, (const void*)(payload.data() + f1_size), f2_size);
      }

      template<>
      void convert(const bytes& payload, test_type2& t) {
         const std::size_t f1_size = sizeof(t.f1);
         const std::size_t f2_size = sizeof(t.f2);
         const std::size_t f3_size = sizeof(t.f3);
         BOOST_REQUIRE_EQUAL(payload.size(), f1_size + f2_size + f3_size);
         std::memcpy((void*)&t.f1, (const void*)payload.data(), f1_size);
         std::memcpy((void*)&t.f2, (const void*)(payload.data() + f1_size), f2_size);
         std::memcpy((void*)&t.f3, (const void*)(payload.data() + f1_size + f2_size), f3_size);
      }
   }
}

BOOST_AUTO_TEST_SUITE(router_tests)
   BOOST_AUTO_TEST_CASE(route_msg)
   {
      generic_message_handler::router<test_type1> msg_router1;
      vector<test_type1> router1_call1;
      std::string received_endpoint;
      scoped_connection con1 = msg_router1.forward_msg.connect([&router1_call1, &received_endpoint](const test_type1& t, const std::string& endpoint) {
         router1_call1.push_back(t);
         received_endpoint = endpoint;
      });

      const test_type1 t1 = { .f1 = 0x12345678, .f2 = 0x2468013579abcdef };

      generic_message msg;
      pack_generic_message(msg, t1);
      msg_router1.route(msg, "localhost");
      BOOST_REQUIRE_EQUAL(router1_call1.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f1, t1.f1);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f2, t1.f2);

      BOOST_REQUIRE_EQUAL(received_endpoint, "localhost");

      vector<test_type1> router1_call2;
      scoped_connection con2 = msg_router1.forward_msg.connect([&router1_call2, &received_endpoint](const test_type1& t, const std::string& endpoint) {
         router1_call2.push_back(t);
         received_endpoint = endpoint;
      });

      const test_type1 t2 = { .f1 = 0x12345677, .f2 = 0x2468013579abcdee };

      pack_generic_message(msg, t2);
      BOOST_REQUIRE(true);
      msg_router1.route(msg, "127.0.0.1");
      BOOST_REQUIRE_EQUAL(router1_call1.size(), 2);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f1, t1.f1);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f2, t1.f2);
      BOOST_REQUIRE_EQUAL(router1_call1[1].f1, t2.f1);
      BOOST_REQUIRE_EQUAL(router1_call1[1].f2, t2.f2);

      BOOST_REQUIRE_EQUAL(router1_call2.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call2[0].f1, t2.f1);
      BOOST_REQUIRE_EQUAL(router1_call2[0].f2, t2.f2);

      BOOST_REQUIRE_EQUAL(received_endpoint, "127.0.0.1");
   }

   BOOST_AUTO_TEST_CASE(routing_handler)
   {
      generic_message_handler handler;
      vector<test_type1> router1_call1;
      scoped_connection con1 = handler.register_msg<test_type1>([&router1_call1](const test_type1& t, const std::string& endpoint) {
         router1_call1.push_back(t);
      });
      vector<test_type1> router1_call2;
      scoped_connection con2 = handler.register_msg<test_type1>([&router1_call2](const test_type1& t, const std::string& endpoint) {
         router1_call2.push_back(t);
      });
      vector<test_type2> router2_call1;
      scoped_connection con3 = handler.register_msg<test_type2>([&router2_call1](const test_type2& t, const std::string& endpoint) {
         router2_call1.push_back(t);
      });

      generic_message msg;

      const test_type1 t1 = { .f1 = 0x12345678, .f2 = 0x2468013579abcdef };

      pack_generic_message(msg, t1);
      handler.route(msg, "localhost");
      BOOST_REQUIRE_EQUAL(router1_call1.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f1, t1.f1);
      BOOST_REQUIRE_EQUAL(router1_call1[0].f2, t1.f2);
      BOOST_REQUIRE_EQUAL(router1_call2.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call2[0].f1, t1.f1);
      BOOST_REQUIRE_EQUAL(router1_call2[0].f2, t1.f2);
      BOOST_REQUIRE_EQUAL(router2_call1.size(), 0);

      const test_type2 t2 = { .f1 = 0x1234567890abcdef, .f2 = 0x24681357, .f3 = 0xa9 };

      pack_generic_message(msg, t2);
      handler.route(msg, "localhost");
      BOOST_REQUIRE_EQUAL(router1_call1.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call2.size(), 1);
      BOOST_REQUIRE_EQUAL(router2_call1.size(), 1);
      BOOST_REQUIRE_EQUAL(router2_call1[0].f1, t2.f1);
      BOOST_REQUIRE_EQUAL(router2_call1[0].f2, t2.f2);
      BOOST_REQUIRE_EQUAL(router2_call1[0].f3, t2.f3);

      const test_type3 t3 = { .f1 = 0x12345678 };

      // does nothing with test_type3
      pack_generic_message(msg, t3);
      handler.route(msg, "localhost");
      BOOST_REQUIRE_EQUAL(router1_call1.size(), 1);
      BOOST_REQUIRE_EQUAL(router1_call2.size(), 1);
      BOOST_REQUIRE_EQUAL(router2_call1.size(), 1);
   }

   BOOST_AUTO_TEST_CASE(registered_types)
   {
      generic_message_handler handler;
      vector<test_type1> router1_call1;
      scoped_connection con1 = handler.register_msg<test_type1>([&router1_call1](const test_type1& t, const std::string& endpoint) {
         router1_call1.push_back(t);
      });
      auto types = handler.get_registered_types();
      BOOST_REQUIRE_EQUAL(types.size(), 1);
      BOOST_REQUIRE_EQUAL(types[0], typeid(test_type1).hash_code());
      vector<test_type1> router1_call2;
      scoped_connection con2 = handler.register_msg<test_type1>([&router1_call2](const test_type1& t, const std::string& endpoint) {
         router1_call2.push_back(t);
      });
      types = handler.get_registered_types();
      BOOST_REQUIRE_EQUAL(types.size(), 1);
      BOOST_REQUIRE_EQUAL(types[0], typeid(test_type1).hash_code());
      vector<test_type2> router2_call1;
      scoped_connection con3 = handler.register_msg<test_type2>([&router2_call1](const test_type2& t, const std::string& endpoint) {
         router2_call1.push_back(t);
      });
      types = handler.get_registered_types();
      BOOST_REQUIRE_EQUAL(types.size(), 2);
      std::unordered_set<uint64_t> expected_types = { typeid(test_type1).hash_code(), typeid(test_type2).hash_code() };
      for (auto type : types) {
         BOOST_REQUIRE_EQUAL(expected_types.count(type), 1);
         expected_types.erase(type);
      }
   }

BOOST_AUTO_TEST_SUITE_END()
