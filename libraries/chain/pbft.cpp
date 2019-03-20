#include <eosio/chain/pbft.hpp>
#include <fc/io/fstream.hpp>
#include <fstream>

namespace eosio {
    namespace chain {

        pbft_controller::pbft_controller(controller &ctrl) : pbft_db(ctrl), state_machine(pbft_db) {
            config.view_change_timeout = 6;
            config.bp_candidate = true;
            datadir = ctrl.state_dir();

            if (!fc::is_directory(datadir))
                fc::create_directories(datadir);

            auto pbft_db_dat = datadir / config::pbftdb_filename;
            if (fc::exists(pbft_db_dat)) {
                string content;
                fc::read_file_contents(pbft_db_dat, content);

                fc::datastream<const char *> ds(content.data(), content.size());
                uint32_t current_view;
                fc::raw::unpack(ds, current_view);
                state_machine.set_current_view(current_view);

                state_machine.set_target_view(state_machine.get_current_view() + 1);
                ilog("current view: ${cv}", ("cv", current_view));
            }

            fc::remove(pbft_db_dat);
        }

        pbft_controller::~pbft_controller() {
            fc::path pbft_db_dat = datadir / config::pbftdb_filename;
            std::ofstream out(pbft_db_dat.generic_string().c_str(),
                              std::ios::out | std::ios::binary | std::ofstream::trunc);

            uint32_t current_view = state_machine.get_current_view();
            fc::raw::pack(out, current_view);
        }

        void pbft_controller::maybe_pbft_prepare() {
            if (!pbft_db.should_send_pbft_msg()) return;
            state_machine.send_prepare();
        }

        void pbft_controller::maybe_pbft_commit() {
            if (!pbft_db.should_send_pbft_msg()) return;
            state_machine.send_commit();
        }

        void pbft_controller::maybe_pbft_view_change() {
            if (!pbft_db.should_send_pbft_msg()) return;
            if (state_machine.get_view_change_timer() <= config.view_change_timeout) {
                if (!state_machine.get_view_changes_cache().empty()) {
                    pbft_db.send_and_add_pbft_view_change(state_machine.get_view_changes_cache());
                }
                state_machine.set_view_change_timer(state_machine.get_view_change_timer() + 1);
            } else {
                state_machine.set_view_change_timer(0);
                state_machine.send_view_change();
            }
        }

        void pbft_controller::on_pbft_prepare(pbft_prepare &p) {
            if (!config.bp_candidate) return;
            state_machine.on_prepare(p);
        }

        void pbft_controller::on_pbft_commit(pbft_commit &c) {
            if (!config.bp_candidate) return;
            state_machine.on_commit(c);
        }

        void pbft_controller::on_pbft_view_change(pbft_view_change &vc) {
            if (!config.bp_candidate) return;
            state_machine.on_view_change(vc);
        }

        void pbft_controller::on_pbft_new_view(pbft_new_view &nv) {
            if (!config.bp_candidate) return;
            state_machine.on_new_view(nv);
        }

        void pbft_controller::send_pbft_checkpoint() {
            if (!pbft_db.should_send_pbft_msg()) return;
            pbft_db.send_pbft_checkpoint();
        }

        void pbft_controller::on_pbft_checkpoint(pbft_checkpoint &cp) {
            pbft_db.add_pbft_checkpoint(cp);
        }

        psm_state::psm_state() = default;

        psm_state::~psm_state() = default;


        psm_machine::psm_machine(pbft_database &pbft_db) : pbft_db(pbft_db) {
            this->set_current(new psm_committed_state);

            this->set_prepares_cache(vector<pbft_prepare>{});
            this->set_commits_cache(vector<pbft_commit>{});
            this->set_view_changes_cache(vector<pbft_view_change>{});

            this->set_prepared_certificate(vector<pbft_prepared_certificate>{});
            this->set_view_changed_certificate(vector<pbft_view_changed_certificate>{});

            this->view_change_timer = 0;
            this->target_view_retries = 0;
            this->current_view = 0;
            this->target_view = this->current_view + 1;
        }

        psm_machine::~psm_machine() = default;

        void psm_machine::on_prepare(pbft_prepare &e) {
            current->on_prepare(this, e, pbft_db);
        }

        void psm_machine::send_prepare() {
            current->send_prepare(this, pbft_db);
        }

        void psm_machine::on_commit(pbft_commit &e) {
            current->on_commit(this, e, pbft_db);
        }

        void psm_machine::send_commit() {
            current->send_commit(this, pbft_db);
        }

        void psm_machine::on_view_change(pbft_view_change &e) {
            current->on_view_change(this, e, pbft_db);
        }

        void psm_machine::send_view_change() {
            current->send_view_change(this, pbft_db);
        }

        void psm_machine::on_new_view(pbft_new_view &e) {
            current->on_new_view(this, e, pbft_db);
        }

        void psm_machine::manually_set_current_view(const uint32_t &current_view) {
            current->manually_set_view(this, current_view);
        }

        /**
         * psm_prepared_state
         */

        psm_prepared_state::psm_prepared_state() {
            pending_commit_local = false;
        }

        psm_prepared_state::~psm_prepared_state() = default;

        void psm_prepared_state::on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) {
            //ignore
        }

        void psm_prepared_state::send_prepare(psm_machine *m, pbft_database &pbft_db) {
            //retry
            if (m->get_prepares_cache().empty()) return;

            pbft_db.send_and_add_pbft_prepare(m->get_prepares_cache(), m->get_current_view());
        }

        void psm_prepared_state::on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) {

            if (e.view < m->get_current_view()) return;

            pbft_db.add_pbft_commit(e);

            //`pending_commit_local` is used to mark committed local status in psm machine;
            //`pbft_db.pending_pbft_lib()` is used to mark commit local status in controller;
            // following logic is implemented to resolve async problem during lib committing;

            if (pbft_db.should_committed() && !pending_commit_local) {
                pbft_db.commit_local();
                pending_commit_local = true;
            }

            if (pending_commit_local && !pbft_db.pending_pbft_lib()) {
                pbft_db.send_pbft_checkpoint();
                m->transit_to_committed_state(this);
            }
        }


        void psm_prepared_state::send_commit(psm_machine *m, pbft_database &pbft_db) {
            auto commits = pbft_db.send_and_add_pbft_commit(m->get_commits_cache(), m->get_current_view());

            if (!commits.empty()) {
                m->set_commits_cache(commits);
            }

            if (pbft_db.should_committed() && !pending_commit_local) {
                pbft_db.commit_local();
                pending_commit_local = true;
            }

            if (pending_commit_local && !pbft_db.pending_pbft_lib()) {
                pbft_db.send_pbft_checkpoint();
                m->transit_to_committed_state(this);
            }
        }

        void psm_prepared_state::on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) {

            if (e.target_view <= m->get_current_view()) return;

            pbft_db.add_pbft_view_change(e);

            //if received >= f+1 view_change on some view, transit to view_change and send view change
            auto target_view = pbft_db.should_view_change();
            if (target_view > 0 && target_view > m->get_current_view()) {
                m->set_target_view(target_view);
                m->transit_to_view_change_state(this);
            }
        }

        void psm_prepared_state::send_view_change(psm_machine *m, pbft_database &pbft_db) {
            m->transit_to_view_change_state(this);
        }

        void psm_prepared_state::on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) {

            if (e.view <= m->get_current_view()) return;

            if (pbft_db.is_valid_new_view(e)) m->transit_to_new_view(e, this);
        }

        void psm_prepared_state::manually_set_view(psm_machine *m, const uint32_t &current_view) {
            m->set_current_view(current_view);
            m->set_target_view(current_view+1);
            m->transit_to_view_change_state(this);
        }

        psm_committed_state::psm_committed_state() {
            pending_commit_local = false;
        }

        psm_committed_state::~psm_committed_state() = default;

        /**
         * psm_committed_state
         */
        void psm_committed_state::on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) {
            //validate
            if (e.view < m->get_current_view()) return;

            //do action add prepare
            pbft_db.add_pbft_prepare(e);

            //if prepare >= 2f+1, transit to prepared
            if (pbft_db.should_prepared()) m->transit_to_prepared_state(this);
        }

        void psm_committed_state::send_prepare(psm_machine *m, pbft_database &pbft_db) {
            auto prepares = pbft_db.send_and_add_pbft_prepare(m->get_prepares_cache(), m->get_current_view());

            if (!prepares.empty()) {
                m->set_prepares_cache(prepares);
            }

            //if prepare >= 2f+1, transit to prepared
            if (pbft_db.should_prepared()) m->transit_to_prepared_state(this);
        }

        void psm_committed_state::on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) {

            if (e.view < m->get_current_view()) return;

            pbft_db.add_pbft_commit(e);
        }

        void psm_committed_state::send_commit(psm_machine *m, pbft_database &pbft_db) {

            if (m->get_commits_cache().empty()) return;
            pbft_db.send_and_add_pbft_commit(m->get_commits_cache(), m->get_current_view());

        }

        void psm_committed_state::on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) {

            if (e.target_view <= m->get_current_view()) return;

            pbft_db.add_pbft_view_change(e);

            //if received >= f+1 view_change on some view, transit to view_change and send view change
            auto new_view = pbft_db.should_view_change();
            if (new_view > 0 && new_view > m->get_current_view()) {
                m->set_target_view(new_view);
                m->transit_to_view_change_state(this);
            }
        }

        void psm_committed_state::send_view_change(psm_machine *m, pbft_database &pbft_db) {
            m->transit_to_view_change_state(this);
        }

        void psm_committed_state::on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) {

            if (e.view <= m->get_current_view()) return;

            if (pbft_db.is_valid_new_view(e)) m->transit_to_new_view(e, this);
        }

        void psm_committed_state::manually_set_view(psm_machine *m, const uint32_t &current_view) {
            m->set_current_view(current_view);
            m->set_target_view(current_view+1);
            m->transit_to_view_change_state(this);
        }

        /**
         * psm_view_change_state
         */
        void psm_view_change_state::on_prepare(psm_machine *m, pbft_prepare &e, pbft_database &pbft_db) {
            //ignore;
        }

        void psm_view_change_state::send_prepare(psm_machine *m, pbft_database &pbft_db) {
            //ignore;
        }

        void psm_view_change_state::on_commit(psm_machine *m, pbft_commit &e, pbft_database &pbft_db) {
            //ignore;
        }

        void psm_view_change_state::send_commit(psm_machine *m, pbft_database &pbft_db) {
            //ignore;
        }

        void psm_view_change_state::on_view_change(psm_machine *m, pbft_view_change &e, pbft_database &pbft_db) {

            //skip from view change state if my lib is higher than my view change state height.
            auto vc = m->get_view_changes_cache();
            if (!vc.empty() && pbft_db.should_stop_view_change(vc.front())) {
                m->transit_to_committed_state(this);
                return;
            }

            if (e.target_view <= m->get_current_view()) return;

            pbft_db.add_pbft_view_change(e);

            //if view_change >= 2f+1, calculate next primary, send new view if is primary
            auto nv = m->get_target_view();
            if (pbft_db.should_new_view(nv) && pbft_db.is_new_primary(nv)) {

                m->set_view_changed_certificate(pbft_db.generate_view_changed_certificate(nv));

                auto new_view = pbft_db.get_proposed_new_view_num();
                if (new_view != nv) return;

                auto nv_msg = pbft_db.send_pbft_new_view(
                        m->get_view_changed_certificate(),
                        new_view);

                if (nv_msg == pbft_new_view{} || !pbft_db.is_valid_new_view(nv_msg)) return;

                m->transit_to_new_view(nv_msg, this);
                return;
            }
        }

        void psm_view_change_state::send_view_change(psm_machine *m, pbft_database &pbft_db) {

            //skip from view change state if my lib is higher than my view change state height.
            auto vc = m->get_view_changes_cache();
            if (!vc.empty() && pbft_db.should_stop_view_change(vc.front())) {
                m->transit_to_committed_state(this);
                return;
            }

            m->send_pbft_view_change();

            //if view_change >= 2f+1, calculate next primary, send new view if is primary
            auto nv = m->get_target_view();
            if (pbft_db.should_new_view(nv) && pbft_db.is_new_primary(nv)) {

                m->set_view_changed_certificate(pbft_db.generate_view_changed_certificate(nv));

                auto new_view = pbft_db.get_proposed_new_view_num();
                if (new_view != nv) return;

                auto nv_msg = pbft_db.send_pbft_new_view(
                        m->get_view_changed_certificate(),
                        new_view);

                if (nv_msg == pbft_new_view{} || !pbft_db.is_valid_new_view(nv_msg)) return;

                m->transit_to_new_view(nv_msg, this);
                return;
            }
        }


        void psm_view_change_state::on_new_view(psm_machine *m, pbft_new_view &e, pbft_database &pbft_db) {

            if (e.view <= m->get_current_view()) return;

            if (pbft_db.is_valid_new_view(e)) m->transit_to_new_view(e, this);
        }

        void psm_view_change_state::manually_set_view(psm_machine *m, const uint32_t &current_view) {
            m->set_current_view(current_view);
            m->set_target_view(current_view+1);
            m->transit_to_view_change_state(this);
        }

        template<typename T>
        void psm_machine::transit_to_committed_state(T const & s) {

            auto nv = pbft_db.get_committed_view();
            if (nv > this->get_current_view()) this->set_current_view(nv);
            this->set_target_view(this->get_current_view() + 1);

            auto prepares = this->pbft_db.send_and_add_pbft_prepare(vector<pbft_prepare>{}, this->get_current_view());
            set_prepares_cache(prepares);

            this->set_view_changes_cache(vector<pbft_view_change>{});
            this->set_view_change_timer(0);

            this->set_current(new psm_committed_state);
            delete s;
        }

        template<typename T>
        void psm_machine::transit_to_prepared_state(T const & s) {

            auto commits = this->pbft_db.send_and_add_pbft_commit(vector<pbft_commit>{}, this->get_current_view());
            set_commits_cache(commits);

            this->set_view_changes_cache(vector<pbft_view_change>{});

            this->set_current(new psm_prepared_state);
            delete s;
        }

        template<typename T>
        void psm_machine::transit_to_view_change_state(T const &s) {

            this->set_commits_cache(vector<pbft_commit>{});
            this->set_prepares_cache(vector<pbft_prepare>{});

            this->set_view_change_timer(0);
            this->set_target_view_retries(0);

            this->set_current(new psm_view_change_state);
            if (pbft_db.should_send_pbft_msg()) this->send_pbft_view_change();

            delete s;
        }

        template<typename T>
        void psm_machine::transit_to_new_view(const pbft_new_view &new_view, T const &s) {

            this->set_current_view(new_view.view);
            this->set_target_view(new_view.view + 1);

            this->set_prepares_cache(vector<pbft_prepare>{});

            this->set_view_change_timer(0);
            this->set_target_view_retries(0);

            this->pbft_db.prune_pbft_index();

            if (!(new_view.stable_checkpoint == pbft_stable_checkpoint{})) {
                for (auto cp :new_view.stable_checkpoint.checkpoints) {
                    try {
                        pbft_db.add_pbft_checkpoint(cp);
                    } catch (...) {
                        wlog("insert checkpoint failed");
                    }
                }
            }

            if (!new_view.prepared.prepares.empty()) {
                for (auto p: new_view.prepared.prepares) {
                    try {
                        pbft_db.add_pbft_prepare(p);
                    } catch (...) {
                        wlog("insert prepare failed");
                    }
                }
                if (pbft_db.should_prepared()) {
                    transit_to_prepared_state(s);
                    return;
                }
            }

            this->set_current(new psm_committed_state);
            delete s;
        }

        void psm_machine::send_pbft_view_change() {

            if (this->get_target_view_retries() == 0) {
                this->set_view_changes_cache(vector<pbft_view_change>{});
                this->set_prepared_certificate(pbft_db.generate_prepared_certificate());
            }

            EOS_ASSERT((this->get_target_view() > this->get_current_view()), pbft_exception,
                       "target view should be always greater than current view");

            if (this->get_target_view_retries() < pow(2, this->get_target_view() - this->get_current_view() - 1)) {
                this->set_target_view_retries(this->get_target_view_retries() + 1);
            } else {
                this->set_target_view_retries(0);
                this->set_target_view(this->get_target_view() + 1);
                this->set_view_changes_cache(vector<pbft_view_change>{});
            }

            auto view_changes = pbft_db.send_and_add_pbft_view_change(
                    this->get_view_changes_cache(),
                    this->get_prepared_certificate(),
                    this->get_current_view(),
                    this->get_target_view());

            if (!view_changes.empty()) {
                this->set_view_changes_cache(view_changes);
            }
        }

        const vector<pbft_prepare> &psm_machine::get_prepares_cache() const {
            return this->cache.prepares_cache;
        }

        void psm_machine::set_prepares_cache(const vector<pbft_prepare> &prepares_cache) {
            this->cache.prepares_cache = prepares_cache;
        }

        const vector<pbft_commit> &psm_machine::get_commits_cache() const {
            return this->cache.commits_cache;
        }

        void psm_machine::set_commits_cache(const vector<pbft_commit> &commits_cache) {
            this->cache.commits_cache = commits_cache;
        }

        const vector<pbft_view_change> &psm_machine::get_view_changes_cache() const {
            return this->cache.view_changes_cache;
        }

        void psm_machine::set_view_changes_cache(const vector<pbft_view_change> &view_changes_cache) {
            this->cache.view_changes_cache = view_changes_cache;
        }

        const uint32_t &psm_machine::get_current_view() const {
            return this->current_view;
        }

        void psm_machine::set_current_view(const uint32_t &current_view) {
            this->current_view = current_view;
        }

        const vector<pbft_prepared_certificate> &psm_machine::get_prepared_certificate() const {
            return this->cache.prepared_certificate;
        }

        void psm_machine::set_prepared_certificate(const vector<pbft_prepared_certificate> &prepared_certificate) {
            this->cache.prepared_certificate = prepared_certificate;
        }

        const vector<pbft_view_changed_certificate> &psm_machine::get_view_changed_certificate() const {
            return this->cache.view_changed_certificate;
        }

        void psm_machine::set_view_changed_certificate(
                const vector<pbft_view_changed_certificate> &view_changed_certificate) {
            this->cache.view_changed_certificate = view_changed_certificate;
        }

        const uint32_t &psm_machine::get_target_view_retries() const {
            return this->target_view_retries;
        }

        void psm_machine::set_target_view_retries(const uint32_t &target_view_reties) {
            this->target_view_retries = target_view_reties;
        }

        const uint32_t &psm_machine::get_target_view() const {
            return this->target_view;
        }

        void psm_machine::set_target_view(const uint32_t &target_view) {
            this->target_view = target_view;
        }

        const uint32_t &psm_machine::get_view_change_timer() const {
            return this->view_change_timer;
        }

        void psm_machine::set_view_change_timer(const uint32_t &view_change_timer) {
            this->view_change_timer = view_change_timer;
        }
    }
}