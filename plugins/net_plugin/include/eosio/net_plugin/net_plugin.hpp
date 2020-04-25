#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/net_plugin/generic_message_handler.hpp>
#include <eosio/net_plugin/protocol.hpp>

namespace eosio {
   using namespace appbase;

   struct connection_status {
      string            peer;
      bool              connecting = false;
      bool              syncing    = false;
      handshake_message last_handshake;
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
        generic_message_types        supported_types( const string& endpoint )const;

        unordered_map<string, generic_message_types> supported_types( bool ignore_endpoints_with_no_support = true )const;

        template<typename T, typename ForwardMessage>
        boost::signals2::scoped_connection register_msg(ForwardMessage forward_msg);

        /**
         * Send the passed in type as a generic_message to all provided endpoints
         *
         * @param t : Type T data to be converted to a generic_message payload
         * @param endpoints : endpoints to send the data to. If none provided, the message
         *                    will be sent to all endpoints that register to receive that
         *                    generic message type.
         */
        template<typename T>
        void send_generic_message(const T& t, const vector<string>& endpoints = vector<string>{});
      private:
        void send(const generic_message& msg, const std::string& type_name, const vector<string>& endpoints);
        std::shared_ptr<class net_plugin_impl> my;
        net::generic_message_handler generic_msg_handler;
   };

   template<typename T, typename ForwardMessage>
   boost::signals2::scoped_connection net_plugin::register_msg(ForwardMessage forward_msg) {
      return generic_msg_handler.register_msg<T>(forward_msg);
   }

   template<typename T>
   void net_plugin::send_generic_message(const T& t, const vector<string>& endpoints) {
      const auto& type_id = typeid(T);
      generic_message msg { .type = type_id.hash_code() };
      net::convert(t, msg.payload);
      send(msg, type_id.name(), endpoints);
   }
}

FC_REFLECT( eosio::connection_status, (peer)(connecting)(syncing)(last_handshake) )
