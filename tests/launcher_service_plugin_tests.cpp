#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/launcher_service_plugin/launcher_service_plugin.hpp>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace eosio::launcher_service;

BOOST_AUTO_TEST_SUITE(launcher_service_plugin_tests)

BOOST_AUTO_TEST_CASE(config_test_mesh)
{
   launcher_config config;
   cluster_def def;

   config.base_port = 1600;
   config.cluster_span = 1000;
   config.node_span = 10;
   config.max_nodes_per_cluster = 100;
   config.max_clusters = 31;
   config.host_name = "localhost";
   config.p2p_listen_addr = "0.0.0.0";

   def.shape = "mesh";
   def.node_count = 4;

   std::string configstr0 = launcher_service_plugin::generate_node_config(config, def, 0);
   std::string configstr1 = launcher_service_plugin::generate_node_config(config, def, 1);
   std::string configstr2 = launcher_service_plugin::generate_node_config(config, def, 2);
   std::string configstr3 = launcher_service_plugin::generate_node_config(config, def, 3);

   BOOST_TEST(configstr0.find("http-server-address = localhost:1600") != std::string::npos);
   BOOST_TEST(configstr0.find("p2p-listen-endpoint = 0.0.0.0:1601") != std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1601") == std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1611") == std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr1.find("http-server-address = localhost:1610") != std::string::npos);
   BOOST_TEST(configstr1.find("p2p-listen-endpoint = 0.0.0.0:1611") != std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1601") != std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1611") == std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr2.find("http-server-address = localhost:1620") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-listen-endpoint = 0.0.0.0:1621") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1601") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1611") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr3.find("http-server-address = localhost:1630") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-listen-endpoint = 0.0.0.0:1631") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1601") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1611") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1621") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1631") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(config_test_star)
{
   launcher_config config;
   cluster_def def;

   config.base_port = 1600;
   config.cluster_span = 1000;
   config.node_span = 10;
   config.max_nodes_per_cluster = 100;
   config.max_clusters = 31;
   config.host_name = "localhost";
   config.p2p_listen_addr = "0.0.0.0";

   def.shape = "star";
   def.node_count = 4;
   def.center_node_id = 1;

   std::string configstr0 = launcher_service_plugin::generate_node_config(config, def, 0);
   std::string configstr1 = launcher_service_plugin::generate_node_config(config, def, 1);
   std::string configstr2 = launcher_service_plugin::generate_node_config(config, def, 2);
   std::string configstr3 = launcher_service_plugin::generate_node_config(config, def, 3);

   BOOST_TEST(configstr0.find("http-server-address = localhost:1600") != std::string::npos);
   BOOST_TEST(configstr0.find("p2p-listen-endpoint = 0.0.0.0:1601") != std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1601") == std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1611") != std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr0.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr1.find("http-server-address = localhost:1610") != std::string::npos);
   BOOST_TEST(configstr1.find("p2p-listen-endpoint = 0.0.0.0:1611") != std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1601") == std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1611") == std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr1.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr2.find("http-server-address = localhost:1620") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-listen-endpoint = 0.0.0.0:1621") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1601") == std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1611") != std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr2.find("p2p-peer-address = localhost:1631") == std::string::npos);

   BOOST_TEST(configstr3.find("http-server-address = localhost:1630") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-listen-endpoint = 0.0.0.0:1631") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1601") == std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1611") != std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1621") == std::string::npos);
   BOOST_TEST(configstr3.find("p2p-peer-address = localhost:1631") == std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
