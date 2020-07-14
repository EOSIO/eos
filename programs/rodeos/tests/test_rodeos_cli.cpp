#include "../streams/rabbitmq.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace eosio::literals;

static void parse_and_check(const std::string& cmdline_arg, const std::string& expected_address,
                            const std::string& expected_queue_name, const std::vector<eosio::name>& expected_routes) {
   std::string              queue_name;
   std::vector<eosio::name> routes;

   auto amqp_address = b1::parse_rabbitmq_address(cmdline_arg, queue_name, routes);

   BOOST_TEST(std::string(amqp_address) == expected_address);
   BOOST_TEST(queue_name == expected_queue_name);
   BOOST_TEST(routes == expected_routes);
}

BOOST_AUTO_TEST_CASE(rabbitmq_address_parsing) {
   // No slashes
   parse_and_check("amqp://user:pass@host", "amqp://user:pass@host/", "", {});

   // One slash (/queue)
   parse_and_check("amqp://user:pass@host/", "amqp://user:pass@host/", "", {});
   parse_and_check("amqp://user:pass@host:1000/queue", "amqp://user:pass@host:1000/", "queue", {});

   // Two slashes (/queue/routes)
   parse_and_check("amqp://user:pass@host//", "amqp://user:pass@host/", "", {});
   parse_and_check("amqp://user:pass@host//r1,r2", "amqp://user:pass@host/", "", { "r1"_n, "r2"_n });
   parse_and_check("amqp://user:pass@host/queue/", "amqp://user:pass@host/", "queue", {});
   parse_and_check("amqp://user:pass@host/queue/*", "amqp://user:pass@host/", "queue", {});
   parse_and_check("amqp://user:pass@host/queue/r1", "amqp://user:pass@host/", "queue", { "r1"_n });
   parse_and_check("amqp://user:pass@host/queue/r1,r2", "amqp://user:pass@host/", "queue", { "r1"_n, "r2"_n });

   // Three slashes (/vhost/queue/routes)
   parse_and_check("amqps://user:pass@host/vhost/queue/*", "amqps://user:pass@host:5671/vhost", "queue", {});
   parse_and_check("amqps://user:pass@host/vhost//*", "amqps://user:pass@host:5671/vhost", "", {});

   // Check that amqp-cpp detects invalid AMQP addresses.
   std::string              queue_name;
   std::vector<eosio::name> routes;

   BOOST_CHECK_EXCEPTION(
         b1::parse_rabbitmq_address("user:pass@host", queue_name, routes), std::runtime_error,
         [](const auto& e) { return std::strstr(e.what(), "AMQP address should start with") != nullptr; });
}
