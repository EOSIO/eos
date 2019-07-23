/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <fc/reflect/reflect.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/topology_plugin/net_metrics.hpp>
#include <eosio/topology_plugin/node_descriptor.hpp>
#include <eosio/topology_plugin/link_descriptor.hpp>
#include <eosio/topology_plugin/messages.hpp>
#include <boost/signals2/signal.hpp>

namespace eosio{
   using namespace appbase;

   class topology_plugin : public appbase::plugin<topology_plugin>
   {
   public:
      topology_plugin();
      virtual ~topology_plugin();

      APPBASE_PLUGIN_REQUIRES((chain_plugin))

      virtual void set_program_options(options_description& cli, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();
#if 0
      /**
       * the forward_* methods populate the list of peers to which this node should send
       * the payload. The assumptions are: blocks flow from the producer to other nodes;
       * transactions flow to the current and as many pending producers as is needed to
       * to ensure the transaction gets blocked before it expires; topology changes are
       * sent everywhere.
       */

      void forward_block(const string& from_producer, vector<string>& to_peers);
      void forward_transaction(vector<string>& to_peers);
#endif
      /**
       * supply the latest round measurements
       */
      void update_samples( const link_sample& ls);

      uint16_t sample_interval_sec ();

      /**
       * receive topology-specific messages from the net module
       */
      void handle_message( topology_message& msg );
      boost::signals2::signal<void(const topology_message& )>  topo_update;
      bool forward_topology_message(const topology_message&, link_id link );
      void make_map_update(topology_message& tm);
      /**
       * adding or removing nodes and links. called from the net plugin, these operations
       * take
       */
      const string& bp_name();
      fc::sha256 gen_long_id (const node_descriptor &desc);
      node_id make_node_id(const fc::sha256 &long_id);
      void set_local_node_id (node_id id);
      node_id add_node(node_descriptor &&desc );
      void drop_node(node_id id);

      link_id add_link(link_descriptor &&desc );
      void drop_link(link_id id);

      /**
       * API functions:
       * nodes returns a json formatted list of node descriptors with roles defined by bit-fields in the
       *      supplied in_roles
       * links returns a json formatted list of link descriptors
       */
      string nodes( const string& in_roles );
      string links( const string& with_node );
      string gen_grid( );
      string get_sample( );

   private:
      std::unique_ptr<class topology_plugin_impl> my;
   };

}
