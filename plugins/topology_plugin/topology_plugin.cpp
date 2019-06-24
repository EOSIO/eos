/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/topology_plugin/topology_plugin.hpp>
#include <eosio/topology_plugin/link_descriptor.hpp>
#include <eosio/topology_plugin/node_descriptor.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/net_plugin/protocol.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/contract_types.hpp>

#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/appender.hpp>
#include <fc/container/flat.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/exception/exception.hpp>

#include <functional>
#include <string>
#include <sstream>

//using namespace eosio::chain::plugin_interface::compat;

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

namespace eosio {
   static appbase::abstract_plugin& _topology_plugin = app().register_plugin<topology_plugin>();

   using std::vector;

   using boost::asio::ip::tcp;
   using boost::asio::ip::address_v4;
   using boost::asio::ip::host_name;
   using boost::multi_index_container;

   using fc::time_point;
   using fc::time_point_sec;
   using eosio::chain::transaction_id_type;
   using eosio::chain::sha256_less;

   /**
    * Link identifies a set of metrics for the network connection between two nodes
    *
    * @attributes
    * id is a unique identifier, a hash of the active and passive node ids
    * active is the id for the "client" connector
    * passive is the id for the "server" connector.
    * hops is a count of intermediate devices, routers, firewalls, etc
    * up holds the metrics bundle for data flow from the "client" to the "server"
    * down holds the metrics bundle for data flow from the "server" to the "client"
    */

   struct topo_link {
      topo_link() : info(), up(), down() {}
      topo_link(link_descriptor &&ld) : up(), down() { info = std::move(ld); }
      link_descriptor info;
      link_metrics   up;
      link_metrics   down;
   };

   struct topo_node {
      topo_node () : info(), links() {}
      topo_node (node_descriptor &&nd) : links() { info = std::move(nd); }

      node_descriptor   info;
      vector<link_id>   links;
      flat_map<node_id,int16_t> distance;
      uint64_t primary_key() const { return info.my_id; }

   };


   constexpr auto link_role_str( link_role role ) {
      switch (role) {
      case blocks: return "blocks";
      case transactions: return "transactions";
      case control: return "control";
      }
   }

   constexpr auto node_role_str( node_role role ) {
      switch (role) {
      case producer : return "producer";
      case backup : return "backup";
      case api : return "api";
      case full : return "full";
      case gateway : return "gateway";
      case special : return "special";
      }
   }


   constexpr uint16_t def_max_watchers = 100;
   constexpr uint16_t def_sample_imterval = 5; // seconds betwteen samples
   constexpr uint16_t def_update = 3;     // number of samples to collect before updating watchers

   /**
    * Topology plugin implementation details.
    *
    */

   class topology_plugin_impl {
   public:
      flat_map<node_id, topo_node> nodes;
      flat_map<link_id, topo_link> links;

      vector<watcher> watchers;
      flat_map<metric_kind, std::string> units;

      node_id gen_id( const node_descriptor &desc );
      link_id gen_id( const link_descriptor &desc );
      int16_t count_hops_i (set<node_id>& seen, node_id to, topo_node& from);
      int16_t count_hops (node_id to);

      uint16_t sample_imterval_sec;
      uint16_t watcher_update;
      uint16_t max_watchers;
      string bp_name;
      node_id my_id;

      bool done = false;
      net_plugin * net_plug = nullptr;
   };

   node_id topology_plugin_impl::gen_id( const node_descriptor &desc ) {
      ostringstream infostrm;
      infostrm << desc.bp_id << desc.host_addr << desc.p2p_port;
      hash<string> hasher;
      return hasher(infostrm.str());
   }

   link_id topology_plugin_impl::gen_id( const link_descriptor &desc ) {
      ostringstream infostrm;
      infostrm << desc.active << desc.passive << link_role_str( desc.role);
      hash<string> hasher;
      return hasher(infostrm.str());
   }

   int16_t topology_plugin_impl::count_hops_i( set<node_id>& seen,
                                               node_id to,
                                               topo_node &from ) {

      if( from.distance.find(to) == from.distance.end() ) {
         for( auto lid : from.links ) {
            auto l = links[lid];
            node_id peer = l.info.active == from.info.my_id ? l.info.passive : l.info.active;
            seen.insert(peer);
            if( peer == to ) {
               from.distance[to] = 1;
            }
            int16_t res = count_hops_i( seen, to, nodes[peer] );
            if (res > -1) {
               res++;
               if( from.distance.find(peer) == from.distance.end() || res < from.distance[peer]) {
                  from.distance[peer] = res;
               }
            }
         }
      }
      return from.distance.find(to) == from.distance.end() ? -1 : from.distance[to];
   }

   int16_t topology_plugin_impl::count_hops (node_id to) {
      if( nodes.find( to ) == nodes.end() ) {
         elog(" no table entry for target node");
         return -1;
      }

      if (to == my_id) {
         return 0;
      }
      topo_node &my_node = nodes[my_id];
      set<node_id> seen;
      return count_hops_i( seen, to, my_node );
   }

   //----------------------------------------------------------------------------------------

   static topology_plugin_impl* my_impl;

   topology_plugin::topology_plugin()
      :my( new topology_plugin_impl ) {
      my_impl = my.get();
   }

   topology_plugin::~topology_plugin() {
   }

   void topology_plugin::set_program_options( options_description& /*cli*/, options_description& cfg )
   {
      cfg.add_options()
         ( "bp-name", bpo::value<string>(), "\"block producer name\" but really any identifier that localizes a set of nodeos hosts" )
         ( "max-watchers", bpo::value<uint16_t>()->default_value(def_max_watchers), "limit the number of watchers refreshed at a time." )
         ( "sample-interval-seconds", bpo::value<uint16_t>()->default_value(def_sample_imterval), "delay between samples")
         ( "watcher-sample-update", bpo::value<uint16_t>()->default_value(def_update), "number of sampling cycles to wait before updating watchers")
         ;
   }

   template<typename T>
   T dejsonify(const string& s) {
      return fc::json::from_string(s).as<T>();
   }

   void topology_plugin::plugin_initialize( const variables_map& options ) {
      ilog("Initialize topology plugin");
      EOS_ASSERT( options.count( "bp-name" ) > 0, chain::plugin_config_exception,
                  "the topology module requires a bp-name be supplied" );
      try {
         my->bp_name = options.at( "bp-name" ).as<string>();
         my->max_watchers = options.at( "max-watchers" ).as<uint16_t>();
         my->sample_imterval_sec = options.at( "sample-interval-seconds").as<uint16_t>();
         EOS_ASSERT( my->sample_imterval_sec > 0, chain::plugin_config_exception, "sampling frequency must be greater than zero.");
         my->watcher_update = options.at( "watcher-sample-update" ).as<uint16_t>();
      }
      catch ( const fc::exception &ex) {
      }
   }

   void topology_plugin::plugin_startup() {
      ilog("Starting topology plugin");
      my->net_plug = app().find_plugin<net_plugin>();
      EOS_ASSERT( my->net_plug != nullptr, chain::plugin_config_exception, "No net plugin found.");
   }

   void topology_plugin::plugin_shutdown() {
      try {
         ilog( "shutdown.." );
         my->done = true;
         ilog( "exit shutdown" );
      }
      FC_CAPTURE_AND_RETHROW()
   }

   node_id topology_plugin::add_node ( node_descriptor&& n ) {
      auto id = my->gen_id (n);
      my->nodes.emplace( id, move(n));

      return id;
   }

   void  topology_plugin::drop_node ( node_id id ) {
      my->nodes.erase( id );
   }

   link_id topology_plugin::add_link ( link_descriptor&& l ) {
      auto id = my->gen_id( l );
      my->links.emplace( id, move(l));
      return id;
   }

   void topology_plugin::drop_link( link_id id ) {
      my->links.erase( id );
   }

   string topology_plugin::nodes( const string& in_roles ) {
      //convert in_roles to list of node roles then add them up
      vector<node_role> roles;
      uint64_t nr = 0;
      for (auto &pr : roles) {
         nr |= pr;
      }
      bool any = nr == 0;
      vector< node_descriptor > specific_nodes;
      for (auto &n : my->nodes) {
         const node_descriptor &nd(n.second.info);
         if ( any || ((nr & nd.role) != 0 ) )
            specific_nodes.push_back(nd);
      }
      ostringstream res;
      res << fc::json::to_pretty_string( specific_nodes ) << ends;

      return res.str();
   }

   string topology_plugin::links( const string&  with_node ) {
      node_descriptor nd; // = unpack with_node
      node_id id = my->gen_id(nd);
      vector< link_descriptor > specific_links;

      for (auto &l : my->links) {
         const link_descriptor &ld(l.second.info);
         if ( ld.active == id || ld.passive == id )
            specific_links.push_back(ld);
      }
      ostringstream res;
      res << fc::json::to_pretty_string( specific_links ) << ends;

      return res.str();

   }

   void topology_plugin::update_samples( link_id link,
                                         const vector<pair<metric_kind,uint32_t> >& down,
                                         const vector<pair<metric_kind,uint32_t> >& up) {
      my->links[link].down.sample(down);
      my->links[link].up.sample(up);
   }

   uint16_t topology_plugin::sample_interval_sec () {
      return my->sample_imterval_sec;
   }

   void topology_plugin::handle_message( topology_message&& msg )
   {
      topology_message_ptr ptr = std::make_shared<topology_message>( std::move( msg ) );
   }

   void topology_plugin::watch( const string& udp_addr, const string& link_def, const string& metrics ) {
   }

   void topology_plugin::unwatch( const string& udp_addr, const string& link_def, const string& metrics ) {
   }

   string topology_plugin::gen_grid( ) {
      return "should be a graphviz compatible description of all nodes and links";
   }

}
