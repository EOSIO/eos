#pragma once
#include <eos/wallet_plugin/wallet_plugin.hpp>
#include <eos/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>

namespace eos {

   using namespace appbase;

   class wallet_api_plugin : public plugin<wallet_api_plugin> {
      public:
        APPBASE_PLUGIN_REQUIRES((wallet_plugin)(http_plugin))

        wallet_api_plugin() = default;
        virtual ~wallet_api_plugin() = default;
        virtual void set_program_options(options_description&, options_description&) override {}
        void plugin_initialize(const variables_map&) {}
        void plugin_startup();
        void plugin_shutdown() {}

      private:
   };

}
