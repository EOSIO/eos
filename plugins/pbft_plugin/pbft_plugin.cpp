#include <memory>

#include <eosio/pbft_plugin/pbft_plugin.hpp>
#include <boost/asio/steady_timer.hpp>
#include <eosio/chain/pbft.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>

namespace eosio {
    static appbase::abstract_plugin &_pbft_plugin = app().register_plugin<pbft_plugin>();
    using namespace std;
    using namespace eosio::chain;

    class pbft_plugin_impl {
    public:
        unique_ptr<boost::asio::steady_timer> prepare_timer;
        unique_ptr<boost::asio::steady_timer> commit_timer;
        unique_ptr<boost::asio::steady_timer> view_change_timer;
        unique_ptr<boost::asio::steady_timer> checkpoint_timer;

        boost::asio::steady_timer::duration prepare_timeout{std::chrono::milliseconds{1000}};
        boost::asio::steady_timer::duration commit_timeout{std::chrono::milliseconds{1000}};
        boost::asio::steady_timer::duration view_change_timeout{std::chrono::seconds{5}};
        boost::asio::steady_timer::duration checkpoint_timeout{std::chrono::seconds{50}};

        void prepare_timer_tick();

        void commit_timer_tick();

        void view_change_timer_tick();

        void checkpoint_timer_tick();

    private:
        bool is_replaying();
        bool is_syncing();
        bool pbft_ready();
    };

    pbft_plugin::pbft_plugin() : my(new pbft_plugin_impl()) {}

    pbft_plugin::~pbft_plugin() = default;

    void pbft_plugin::set_program_options(options_description &, options_description &cfg) {
    }

    void pbft_plugin::plugin_initialize(const variables_map &options) {
        ilog("Initialize pbft plugin");
        my->prepare_timer = std::make_unique<boost::asio::steady_timer>(app().get_io_service());
        my->commit_timer = std::make_unique<boost::asio::steady_timer>(app().get_io_service());
        my->view_change_timer = std::make_unique<boost::asio::steady_timer>(app().get_io_service());
        my->checkpoint_timer = std::make_unique<boost::asio::steady_timer>(app().get_io_service());
    }

    void pbft_plugin::plugin_startup() {
        my->prepare_timer_tick();
        my->commit_timer_tick();
        my->view_change_timer_tick();
        my->checkpoint_timer_tick();
    }

    void pbft_plugin::plugin_shutdown() {
    }

    void pbft_plugin_impl::prepare_timer_tick() {
        chain::pbft_controller &pbft_ctrl = app().get_plugin<chain_plugin>().pbft_ctrl();
        prepare_timer->expires_from_now(prepare_timeout);
        prepare_timer->async_wait([&](boost::system::error_code ec) {
            prepare_timer_tick();
            if (ec) {
                wlog ("pbft plugin prepare timer tick error: ${m}", ("m", ec.message()));
            } else {
                if (pbft_ready()) pbft_ctrl.maybe_pbft_prepare();
            }
        });
    }

    void pbft_plugin_impl::commit_timer_tick() {
        chain::pbft_controller &pbft_ctrl = app().get_plugin<chain_plugin>().pbft_ctrl();
        commit_timer->expires_from_now(commit_timeout);
        commit_timer->async_wait([&](boost::system::error_code ec) {
            commit_timer_tick();
            if (ec) {
                wlog ("pbft plugin commit timer tick error: ${m}", ("m", ec.message()));
            } else {
                if (pbft_ready()) pbft_ctrl.maybe_pbft_commit();
            }
        });
    }

    void pbft_plugin_impl::view_change_timer_tick() {
        chain::pbft_controller &pbft_ctrl = app().get_plugin<chain_plugin>().pbft_ctrl();
        try {
            view_change_timer->cancel();
        } catch (boost::system::system_error &e) {
            elog("view change timer cancel error: ${e}", ("e", e.what()));
        }
        view_change_timer->expires_from_now(view_change_timeout);
        view_change_timer->async_wait([&](boost::system::error_code ec) {
            view_change_timer_tick();
            if (ec) {
                wlog ("pbft plugin view change timer tick error: ${m}", ("m", ec.message()));
            } else {
                if (pbft_ready()) pbft_ctrl.maybe_pbft_view_change();
            }
        });
    }

    void pbft_plugin_impl::checkpoint_timer_tick() {
        chain::pbft_controller &pbft_ctrl = app().get_plugin<chain_plugin>().pbft_ctrl();
        checkpoint_timer->expires_from_now(checkpoint_timeout);
        checkpoint_timer->async_wait([&](boost::system::error_code ec) {
            checkpoint_timer_tick();
            if (ec) {
                wlog ("pbft plugin checkpoint timer tick error: ${m}", ("m", ec.message()));
            } else {
                if (pbft_ready()) pbft_ctrl.send_pbft_checkpoint();
            }
        });
    }

    bool pbft_plugin_impl::is_replaying() {
        return app().get_plugin<chain_plugin>().chain().is_replaying();
    }

    bool pbft_plugin_impl::is_syncing() {
        return app().get_plugin<net_plugin>().is_syncing();
    }

    bool pbft_plugin_impl::pbft_ready() {
        // only trigger pbft related logic if I am in sync and replayed.
        return (!is_syncing() && !is_replaying());
    }
}
