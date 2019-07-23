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
      topo_node () : info(), links(), distance() {}
      topo_node (node_descriptor &&nd) : links() { info = std::move(nd); }

      node_descriptor   info;
      vector<link_id>   links;
      flat_map<node_id,int16_t> distance;
      string dot;
      uint64_t primary_key() const { return info.my_id; }

      string dot_label() {
         if (dot.empty()) {
            ostringstream dt;
            dt << info.location << "(" << info.my_id << ")";
            dot = dt.str();
         }
         return dot;
      }
   };


   constexpr auto link_role_str( link_role role ) {
      switch (role) {
      case blocks: return "blocks";
      case transactions: return "transactions";
      case control: return "control";
      case combined: return "combined";
      }
      return "error";
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
      return "error";
   }


   const fc::string logger_name("topology_plugin_impl");
   fc::logger topo_logger;

   #if 0
   std::string peer_log_format;

#define peer_dlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::debug ) ) \
      logger.log( FC_LOG_MESSAGE( debug, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_ilog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::info ) ) \
      logger.log( FC_LOG_MESSAGE( info, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_wlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::warn ) ) \
      logger.log( FC_LOG_MESSAGE( warn, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_elog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::error ) ) \
      logger.log( FC_LOG_MESSAGE( error, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant())) ); \
  FC_MULTILINE_MACRO_END
#endif

   template<class enum_type, class=typename std::enable_if<std::is_enum<enum_type>::value>::type>
   inline enum_type& operator|=(enum_type& lhs, const enum_type& rhs)
   {
      using T = std::underlying_type_t <enum_type>;
      return lhs = static_cast<enum_type>(static_cast<T>(lhs) | static_cast<T>(rhs));
   }

   constexpr uint16_t def_sample_imterval = 5; // seconds betwteen samples

   /**
    * Topology plugin implementation details.
    *
    */

   class topology_plugin_impl {
   public:
      flat_map<node_id, topo_node> nodes;
      flat_map<link_id, topo_link> links;

      flat_map<metric_kind, std::string> units;

      fc::sha256 gen_long_id( const node_descriptor &desc );
      node_id make_node_id( const fc::sha256 &long_id);
      node_id gen_id( const node_descriptor &desc );
      link_id gen_id( const link_descriptor &desc );
      int16_t count_hops_i (set<node_id>& seen, node_id to, topo_node& from);
      int16_t count_hops (node_id to);
      node_id peer_node (link_id onlink, node_id from);
      node_id add_node( node_descriptor&& n );
      void    drop_node( node_id id );
      link_id add_link( link_descriptor&& l );
      void    drop_link( link_id id );

      void    update_samples( const link_sample &ls, bool flip );
      void    update_map( const map_update &mu );

      uint16_t sample_imterval_sec;
      string bp_name;
      node_id local_node_id;
      bool done = false;
      net_plugin * net_plug = nullptr;

      std::mutex table_mtx;
   };

   fc::sha256 topology_plugin_impl::gen_long_id( const node_descriptor &desc ) {
      ostringstream infostrm;
      infostrm << desc.location << desc.role << desc.version;
      string istr = infostrm.str();
      return fc::sha256::hash(istr.c_str(), istr.size());
   }

   node_id topology_plugin_impl::make_node_id( const fc::sha256 &long_id ) {
      return long_id.data()[0];
   }

   node_id topology_plugin_impl::gen_id( const node_descriptor &desc ) {
      fc::sha256 lid = gen_long_id (desc);
      return make_node_id( lid );
   }

   link_id topology_plugin_impl::gen_id( const link_descriptor &desc ) {
      ostringstream infostrm;
      infostrm << desc.active << desc.passive << link_role_str( desc.role);
      hash<string> hasher;
      return hasher(infostrm.str());
   }

   node_id topology_plugin_impl::add_node( node_descriptor&& n ) {
      if (n.my_id == 0) {
         n.my_id = gen_id (n);
      }
      ilog( "adding id = ${id} loc = ${loc}",("id", n.my_id)("loc",n.location));
      node_id id = n.my_id;
      if (nodes.find(n.my_id) == nodes.end()) {
         std::lock_guard<std::mutex> g( table_mtx );

         topo_node tn(move(n));

         for (auto &np : nodes) {
            np.second.distance[id] = count_hops(id);
            tn.distance[np.first] = np.second.distance[id];
         }
         nodes.emplace( id, move(tn));
      }
      else {
         ilog("redundant invocation");
      }
      return id;
   }

   void topology_plugin_impl::drop_node( node_id id ) {
      nodes.erase( id );
   }

   link_id topology_plugin_impl::add_link( link_descriptor&& l ) {
      auto id = gen_id( l );
      l.my_id = id;
      ilog ("adding a link ${id} from ${e1} to ${e2}",("id",id)("e1",l.passive)("e2", l.active));
      {
         auto mn = nodes.find(l.active);
         if ( mn != nodes.end()) {
            std::lock_guard<std::mutex> g( table_mtx );

            mn->second.links.push_back( l.my_id );
         }
         else {
            ilog ("no node found for active peer ${la}", ("la",l.active));
         }
      }
      {
         auto mn = nodes.find(l.passive);
         if ( mn != nodes.end()) {
            std::lock_guard<std::mutex> g( table_mtx );
            mn->second.links.push_back( l.my_id );
         }
         else {
            ilog ("no node found for passive peer ${la}", ("la",l.active));
         }
      }
      {
         std::lock_guard<std::mutex> g( table_mtx );
         links.emplace( id, move(l));
      }
      return id;
   }

   void topology_plugin_impl::drop_link( link_id id ) {
      links.erase( id );
   }

   int16_t topology_plugin_impl::count_hops_i( set<node_id>& seen,
                                               node_id to,
                                               topo_node &from ) {

      if( from.distance.find(to) == from.distance.end() ) {
         for( auto lid : from.links ) {
            if(links.find(lid) == links.end()) {
               ilog( "link id ${id} not found", ("id",lid));
               continue;
            }
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
         fc_elog(topo_logger, " no table entry for target node");
         return -1;
      }

      if (to == local_node_id) {
         return 0;
      }
      auto &my_node = nodes[local_node_id];
      set<node_id> seen;
      return count_hops_i( seen, to, my_node );
   }

   node_id topology_plugin_impl::peer_node (link_id onlink, node_id from)
   {
      if (links.find(onlink) == links.end()) {
         ilog("link id ${id} not found",("id",onlink));
         return 0;
      }
      return links[onlink].info.active == from ? links[onlink].info.passive : links[onlink].info.active;
   }

   void topology_plugin_impl::update_samples( const link_sample &ls, bool flip) {
      if (links.find(ls.link) != links.end()) {
         links[ls.link].down.sample(flip ? ls.up : ls.down);
         links[ls.link].up.sample(flip ? ls.down : ls.up);
      }
   }

   void topology_plugin_impl::update_map( const map_update &mu ) {
      for( auto &an : mu.add_nodes ) {
         node_descriptor nd = an;
         add_node(move(nd));
      }
      for( auto &al : mu.add_links ) {
         link_descriptor ld = al;
         ilog( "add link with id ${id}",("id",ld.my_id));
         add_link(move(ld));
      }
      for( auto &dn : mu.drop_nodes ) {
         drop_node(dn);
      }
      for( auto &dl : mu.drop_links ) {
         drop_link(dl);
      }
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
         ( "sample-interval-seconds", bpo::value<uint16_t>()->default_value(def_sample_imterval), "delay between samples")
         ;
   }

   template<typename T>
   T dejsonify(const string& s) {
      return fc::json::from_string(s).as<T>();
   }

   void topology_plugin::plugin_initialize( const variables_map& options ) {
      fc_ilog(topo_logger, "Initialize topology plugin");
      EOS_ASSERT( options.count( "bp-name" ) > 0, chain::plugin_config_exception,
                  "the topology module requires a bp-name be supplied" );
      try {
         my->bp_name = options.at( "bp-name" ).as<string>();
         my->sample_imterval_sec = options.at( "sample-interval-seconds").as<uint16_t>();
         EOS_ASSERT( my->sample_imterval_sec > 0, chain::plugin_config_exception, "sampling frequency must be greater than zero.");
      }
      catch ( const fc::exception &ex) {
      }
   }

   void topology_plugin::plugin_startup() {
      fc_ilog(topo_logger, "Starting topology plugin");
      my->net_plug = app().find_plugin<net_plugin>();
      EOS_ASSERT( my->net_plug != nullptr, chain::plugin_config_exception, "No net plugin found.");
   }

   void topology_plugin::plugin_shutdown() {
      try {
         fc_ilog(topo_logger,  "shutdown.." );
         my->done = true;
         fc_ilog(topo_logger,  "exit shutdown" );
      }
      FC_CAPTURE_AND_RETHROW()
   }


   const string& topology_plugin::bp_name () {
      return my->bp_name;
   }

   void topology_plugin::make_map_update ( topology_message &tm ) {
      tm.origin = my->local_node_id;
      tm.fwds = 0;
      tm.ttl = 1;

      map_update mu;
      for (auto &np : my->nodes) {
         mu.add_nodes.push_back(np.second.info);
      }
      for (auto &lp : my->links) {
         mu.add_links.push_back(lp.second.info);
      }
      tm.payload.emplace_back(move(mu));

   }

   fc::sha256 topology_plugin::gen_long_id( const node_descriptor &desc ) {
      return my->gen_long_id(desc);
   }

   node_id topology_plugin::make_node_id( const fc::sha256 &long_id ) {
      return my->make_node_id(long_id);
   }

   void topology_plugin::set_local_node_id( node_id id) {
      my->local_node_id = id;
   }

   node_id topology_plugin::add_node( node_descriptor&& n ) {
      return my->add_node(move(n));
   }

   void  topology_plugin::drop_node( node_id id ) {
      my->drop_node( id );
   }

   link_id topology_plugin::add_link( link_descriptor&& l ) {
      return my->add_link(move(l));
   }

   void topology_plugin::drop_link( link_id id ) {
      my->drop_link(id);
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

   void topology_plugin::update_samples( const link_sample &ls) {
      my->update_samples( ls, false );
      topology_message tm;
      tm.origin = my->local_node_id;
      tm.destination = my->peer_node(ls.link, tm.origin);
      tm.fwds = 0;
      tm.ttl = 1;
      tm.payload.push_back( ls );
      ilog("sending new sample update message");
      topo_update(tm);
   }

   uint16_t topology_plugin::sample_interval_sec () {
      return my->sample_imterval_sec;
   }

   class topo_msg_handler : public fc::visitor<void> {
   public:
      void operator()( const map_update& msg) {
         ilog("got a map update message");
         my_impl->update_map( msg );
      }

      void operator()( const link_sample& msg) {
         ilog("got a link sample message");
         my_impl->update_samples( msg, true );
      }
   };

   void topology_plugin::handle_message( topology_message& msg )
   {
      ilog("handling a new topology message");
      topology_message_ptr ptr = std::make_shared<topology_message>( msg );
      for (auto &p : ptr->payload) {
         topo_msg_handler tm;
         p.visit( tm );
      }
      ptr->fwds++;
      if( ptr->ttl > ptr->fwds ) {
         ilog("forwarding topology message. ttl = ${t}. fwds = ${f}",("t",ptr->ttl)("f",ptr->fwds));
         topo_update(*ptr);
      }
   }

   // deciding whether or not to forward depends on:
   // 1. did this already come from us?
   // 2. are we on the shortest path?
   // 3. is the forward count equal to our number of hops from the origin?
   bool topology_plugin::forward_topology_message(const topology_message& tm, link_id li ) {
      auto &link = my->links[li];
      node_id peerid = link.info.active == my->local_node_id ? link.info.passive : link.info.active;
      elog ("origin = ${id} dest = ${to}, forwards = ${f}, ttl = ${tt}", ("id",tm.origin)("f",tm.fwds)("tt",tm.ttl)("to",tm.destination));
      if( tm.origin == my->local_node_id && tm.fwds > 0 ) {
         return false;
      }

      if (my->nodes[tm.origin].distance[my->local_node_id] < tm.fwds) {
         elog ("message has too many hops, distance = ${d}, ttl = ${t}",
                  ("d",my->nodes[tm.origin].distance[my->local_node_id])("t",tm.fwds));
         return false;
      }
      return true;
   }


   string topology_plugin::gen_grid( ) {
      ostringstream df;
      df << "digraph G\n{\nlayout=\"circo\";" << endl;
      set<link_id> seen;
      //      ilog("gen grid, node count = ${c}",("c",my->nodes.size()));
      for (auto &tnode : my->nodes) {
         // ilog("tnode ${n} has ${l} links",("n",tnode.second.info.location)("l",tnode.second.links.size()));
         for (const auto &l : tnode.second.links) {
            if (seen.find(l) != seen.end()) {
               ilog("link id ${id} already seen",("id",l));
               continue;
            }
            auto tlink = my->links.find(l);
            if ( tlink == my->links.end() ) {
               ilog( "did not find link id ${id}", ("id",l));
               continue;
            }
            seen.insert(tlink->second.info.my_id);
            // ilog("adding link ${l} aka ${id}",("id",tlink.info.my_id)("l",l));
            string alabel, plabel;
            if (tlink->second.info.passive == tnode.first) {
               alabel = my->nodes[tlink->second.info.active].dot_label();
               plabel = tnode.second.dot_label();
            } else {
               plabel = my->nodes[tlink->second.info.active].dot_label();
               alabel = tnode.second.dot_label();
            }

            df << alabel
               << " -> " << plabel
               << " [dir=\"forward\"];" << endl;
         }
      }
      df << "}\n";
      return df.str();

   }

   string topology_plugin::get_sample( ) {
      ostringstream df;
      df << "{ \"links\" = [" << endl;
      for (auto &tlink : my->links) {
         df << "{ \"key\" = \"" << tlink.first << "\"," << endl;
         df << "\"value\" = " << fc::json::to_pretty_string( tlink.second ) << "}" << endl;
      }
      df << "]}" << endl;

      return df.str();

   }

}

FC_REFLECT(eosio::topo_link,(info)(up)(down))
FC_REFLECT(eosio::topo_node,(info)(links)(distance))
