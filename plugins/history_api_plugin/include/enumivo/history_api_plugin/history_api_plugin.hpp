/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#pragma once
#include <enumivo/history_plugin.hpp>
#include <enumivo/chain_plugin/chain_plugin.hpp>
#include <enumivo/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>

namespace enumivo {

   using namespace appbase;

   class history_api_plugin : public plugin<history_api_plugin> {
      public:
        APPBASE_PLUGIN_REQUIRES((history_plugin)(chain_plugin)(http_plugin))

        history_api_plugin();
        virtual ~history_api_plugin();

        virtual void set_program_options(options_description&, options_description&) override;

        void plugin_initialize(const variables_map&);
        void plugin_startup();
        void plugin_shutdown();

      private:
   };

}
