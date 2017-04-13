#pragma once
#include <appbase/application.hpp>
#include <eos/chain/database.hpp>

namespace eos {
   using eos::chain::database;
   using std::unique_ptr;
   using namespace appbase;

   class chain_plugin : public plugin<chain_plugin>
   {
      public:
        APPBASE_PLUGIN_REQUIRES()

        chain_plugin();
        virtual ~chain_plugin();

        virtual void set_program_options(options_description& cli, options_description& cfg) override;

        void plugin_initialize(const variables_map& options);
        void plugin_startup();
        void plugin_shutdown();

        bool accept_block(const chain::signed_block& block, bool currently_syncing);
        void accept_transaction(const chain::signed_transaction& trx);

        bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

        database& db();

      private:
        unique_ptr<class chain_plugin_impl> my;
   };

}
