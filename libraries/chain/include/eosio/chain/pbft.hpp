#pragma once

#include <eosio/chain/pbft_database.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <chrono>

namespace eosio {
    namespace chain {
        using namespace std;
        using namespace fc;

        struct psm_cache {
            vector<pbft_prepare> prepares_cache;
            vector<pbft_commit> commits_cache;
            vector<pbft_view_change> view_changes_cache;
            vector<pbft_prepared_certificate> prepared_certificate;
            vector<pbft_view_changed_certificate> view_changed_certificate;
        };

        class psm_machine {
            class psm_state *current;

        public:
            explicit psm_machine(pbft_database& pbft_db);
            ~psm_machine();

            void set_current(psm_state *s) {
                current = s;
            }

            void on_prepare(pbft_prepare &e);
            void send_prepare();

            void on_commit(pbft_commit &e);
            void send_commit();

            void on_view_change(pbft_view_change &e);
            void send_view_change();

            void on_new_view(pbft_new_view &e);

            template<typename T>
            void transit_to_committed_state(T const & s);

            template<typename T>
            void transit_to_prepared_state(T const & s);

            void send_pbft_view_change();

            template<typename T>
            void transit_to_view_change_state(T const & s);

            template<typename T>
            void transit_to_new_view(const pbft_new_view &new_view, T const &s);

            const vector<pbft_prepare> &get_prepares_cache() const;

            void set_prepares_cache(const vector<pbft_prepare> &prepares_cache);

            const vector<pbft_commit> &get_commits_cache() const;

            void set_commits_cache(const vector<pbft_commit> &commits_cache);

            const vector<pbft_view_change> &get_view_changes_cache() const;

            void set_view_changes_cache(const vector<pbft_view_change> &view_changes_cache);

            const uint32_t &get_current_view() const;

            void set_current_view(const uint32_t &current_view);

            const vector<pbft_prepared_certificate> &get_prepared_certificate() const;

            void set_prepared_certificate(const vector<pbft_prepared_certificate> &prepared_certificate);

            const vector<pbft_view_changed_certificate> &get_view_changed_certificate() const;

            void set_view_changed_certificate(const vector<pbft_view_changed_certificate> &view_changed_certificate);

            const uint32_t &get_target_view_retries() const;

            void set_target_view_retries(const uint32_t &target_view_reties);

            const uint32_t &get_target_view() const;

            void set_target_view(const uint32_t &target_view);

            const uint32_t &get_view_change_timer() const;

            void set_view_change_timer(const uint32_t &view_change_timer);

            void manually_set_current_view(const uint32_t &current_view);

        protected:
            psm_cache cache;
            uint32_t current_view;
            uint32_t target_view_retries;
            uint32_t target_view;
            uint32_t view_change_timer;

        private:
            pbft_database &pbft_db;

        };

        class psm_state {

        public:
            psm_state();
            ~psm_state();

            virtual void on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) = 0;

            virtual void send_prepare(psm_machine *m, pbft_database &pbft_db) = 0;

            virtual void on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) = 0;

            virtual void send_commit(psm_machine *m, pbft_database &pbft_db) = 0;

            virtual void on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) = 0;

            virtual void send_view_change(psm_machine *m, pbft_database &pbft_db) = 0;

            virtual void on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) = 0;

            virtual void manually_set_view(psm_machine *m, const uint32_t &view) = 0;

        };

        class psm_prepared_state final: public psm_state {

        public:
            psm_prepared_state();
            ~psm_prepared_state();

            void on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) override;

            void send_prepare(psm_machine *m, pbft_database &pbft_db) override;

            void on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) override;

            void send_commit(psm_machine *m, pbft_database &pbft_db) override;

            void on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) override;

            void send_view_change(psm_machine *m, pbft_database &pbft_db) override;

            void on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) override;

            void manually_set_view(psm_machine *m, const uint32_t &view) override;

            bool pending_commit_local;

        };

        class psm_committed_state final: public psm_state {
        public:
            psm_committed_state();
            ~psm_committed_state();

            void on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) override;

            void send_prepare(psm_machine *m, pbft_database &pbft_db) override;

            void on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) override;

            void send_commit(psm_machine *m, pbft_database &pbft_db) override;

            void on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) override;

            void send_view_change(psm_machine *m, pbft_database &pbft_db) override;

            void on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) override;

            void manually_set_view(psm_machine *m, const uint32_t &view) override;

            bool pending_commit_local;
        };

        class psm_view_change_state final: public psm_state {
        public:
            void on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) override;

            void send_prepare(psm_machine *m, pbft_database &pbft_db) override;

            void on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) override;

            void send_commit(psm_machine *m, pbft_database &pbft_db) override;

            void on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) override;

            void send_view_change(psm_machine *m, pbft_database &pbft_db) override;

            void on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) override;

            void manually_set_view(psm_machine *m, const uint32_t &view) override;
        };

        struct pbft_config {
            uint32_t view_change_timeout = 6;
            bool     bp_candidate = false;
        };

        class pbft_controller {
        public:
            pbft_controller(controller& ctrl);
            ~pbft_controller();

            pbft_database pbft_db;
            psm_machine state_machine;
            pbft_config config;

            void maybe_pbft_prepare();
            void maybe_pbft_commit();
            void maybe_pbft_view_change();
            void send_pbft_checkpoint();

            void on_pbft_prepare(pbft_prepare &p);
            void on_pbft_commit(pbft_commit &c);
            void on_pbft_view_change(pbft_view_change &vc);
            void on_pbft_new_view(pbft_new_view &nv);
            void on_pbft_checkpoint(pbft_checkpoint &cp);

        private:
            fc::path datadir;


        };
    }
} /// namespace eosio::chain

FC_REFLECT(eosio::chain::pbft_controller, (pbft_db)(state_machine)(config))