#pragma once

#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace kafka {
class kafka; // forward declaration
}

namespace eosio {

using namespace appbase;

class kafka_plugin : public appbase::plugin<kafka_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((kafka_plugin))

    kafka_plugin();
    virtual ~kafka_plugin();

    void set_program_options(options_description&, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

private:
    bool configured_{};

    chain_plugin* chain_plugin_{};

    boost::signals2::connection block_conn_;
    boost::signals2::connection irreversible_block_conn_;
    boost::signals2::connection transaction_conn_;

    std::atomic<bool> start_sync_{false};

    std::unique_ptr<class kafka::kafka> kafka_;
};

}
