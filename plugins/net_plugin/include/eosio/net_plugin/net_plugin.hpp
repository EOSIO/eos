#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/net_plugin/protocol.hpp>

namespace eosio {
   using namespace appbase;

   enum class connection_state : char {
      never_connected,
      connecting,
      handshaking,
      connected,
      closed,
      go_away
   };

   struct connection_status {
      string            peer;
      bool              connecting = false;
      bool              syncing    = false;
      uint32_t          last_block_sent{0};
      uint32_t          last_block_received{0};
      connection_state  last_status{connection_state::never_connected};
      handshake_message last_handshake_recv;
      handshake_message last_handshake_sent;
   };

   class net_plugin : public appbase::plugin<net_plugin>
   {
      public:
        net_plugin();
        virtual ~net_plugin();

        APPBASE_PLUGIN_REQUIRES((chain_plugin))
        virtual void set_program_options(options_description& cli, options_description& cfg) override;
        void handle_sighup() override;

        void plugin_initialize(const variables_map& options);
        void plugin_startup();
        void plugin_shutdown();

        string                       connect( const string& endpoint );
        string                       disconnect( const string& endpoint );
        optional<connection_status>  status( const string& endpoint )const;
        vector<connection_status>    connections()const;

      private:
        std::shared_ptr<class net_plugin_impl> my;
   };

}

FC_REFLECT_ENUM( eosio::connection_state, (never_connected)(connecting)(handshaking)(connected)(closed)(go_away))
FC_REFLECT( eosio::connection_status, (peer)(connecting)(syncing)(last_block_sent)(last_block_received)(last_status)(last_handshake_recv)(last_handshake_sent) )
