#include <eosio/chain/pbft_database.hpp>
#include <fc/io/fstream.hpp>
#include <fstream>
#include <eosio/chain/global_property_object.hpp>

namespace eosio {
    namespace chain {

        pbft_database::pbft_database(controller &ctrl) :
                ctrl(ctrl),
                view_state_index(pbft_view_state_multi_index_type{}) {
            checkpoint_index = pbft_checkpoint_state_multi_index_type{};
            pbft_db_dir = ctrl.state_dir();
            checkpoints_dir = ctrl.blocks_dir();

            if (!fc::is_directory(pbft_db_dir)) fc::create_directories(pbft_db_dir);

            auto pbft_db_dat = pbft_db_dir / config::pbftdb_filename;
            if (fc::exists(pbft_db_dat)) {
                string content;
                fc::read_file_contents(pbft_db_dat, content);

                fc::datastream<const char *> ds(content.data(), content.size());

                // keep these unused variables.
                uint32_t current_view;
                fc::raw::unpack(ds, current_view);

                unsigned_int size;
                fc::raw::unpack(ds, size);
                for (uint32_t i = 0, n = size.value; i < n; ++i) {
                    pbft_state s;
                    fc::raw::unpack(ds, s);
                    set(std::make_shared<pbft_state>(move(s)));
                }
                ilog("pbft index size: ${s}", ("s", pbft_state_index.size()));
            } else {
                pbft_state_index = pbft_state_multi_index_type{};
            }

            if (!fc::is_directory(checkpoints_dir)) fc::create_directories(checkpoints_dir);

            auto checkpoints_db = checkpoints_dir / config::checkpoints_filename;
            if (fc::exists(checkpoints_db)) {
                string content;
                fc::read_file_contents(checkpoints_db, content);

                fc::datastream<const char *> ds(content.data(), content.size());

                unsigned_int checkpoint_size;
                fc::raw::unpack(ds, checkpoint_size);
                for (uint32_t j = 0, m = checkpoint_size.value; j < m; ++j) {
                    pbft_checkpoint_state cs;
                    fc::raw::unpack(ds, cs);
                    set(std::make_shared<pbft_checkpoint_state>(move(cs)));
                }
                ilog("checkpoint index size: ${cs}", ("cs", checkpoint_index.size()));
            } else {
                checkpoint_index = pbft_checkpoint_state_multi_index_type{};
            }
        }

        void pbft_database::close() {


            fc::path checkpoints_db = checkpoints_dir / config::checkpoints_filename;
            std::ofstream c_out(checkpoints_db.generic_string().c_str(),
                                std::ios::out | std::ios::binary | std::ofstream::trunc);

            uint32_t num_records_in_checkpoint_db = checkpoint_index.size();
            fc::raw::pack(c_out, unsigned_int{num_records_in_checkpoint_db});

            if (!checkpoint_index.empty()) {
                for (const auto &s: checkpoint_index) {
                    fc::raw::pack(c_out, *s);
                }
            }

            fc::path pbft_db_dat = pbft_db_dir / config::pbftdb_filename;
            std::ofstream out(pbft_db_dat.generic_string().c_str(),
                              std::ios::out | std::ios::binary | std::ofstream::app);
            uint32_t num_records_in_db = pbft_state_index.size();
            fc::raw::pack(out, unsigned_int{num_records_in_db});

            if (!pbft_state_index.empty()) {
                for (const auto &s : pbft_state_index) {
                    fc::raw::pack(out, *s);
                }
            }
            pbft_state_index.clear();
            checkpoint_index.clear();
        }

        pbft_database::~pbft_database() {
            close();
        }


        void pbft_database::add_pbft_prepare(pbft_prepare &p) {

            if (!is_valid_prepare(p)) return;

            auto &by_block_id_index = pbft_state_index.get<by_block_id>();

            auto current = ctrl.fetch_block_state_by_id(p.block_id);

            while ((current) && (current->block_num > ctrl.last_irreversible_block_num())) {
                auto curr_itr = by_block_id_index.find(current->id);

                if (curr_itr == by_block_id_index.end()) {
                    try {
                        auto curr_ps = pbft_state{current->id, current->block_num, {p}};
                        auto curr_psp = make_shared<pbft_state>(curr_ps);
                        pbft_state_index.insert(curr_psp);
                    } catch (...) {
                        EOS_ASSERT(false, pbft_exception, "prepare insert failure: ${p}", ("p", p));
                    }
                } else {
                    auto prepares = (*curr_itr)->prepares;
                    auto p_itr = find_if(prepares.begin(), prepares.end(),
                                         [&](const pbft_prepare &prep) {
                                             return prep.public_key == p.public_key && prep.view == p.view;
                                         });
                    if (p_itr == prepares.end()) {
                        by_block_id_index.modify(curr_itr, [&](const pbft_state_ptr &psp) {
                            psp->prepares.emplace_back(p);
                            std::sort(psp->prepares.begin(), psp->prepares.end(), less<>());
                        });
                    }
                }
                curr_itr = by_block_id_index.find(current->id);
                if (curr_itr == by_block_id_index.end()) return;

                auto prepares = (*curr_itr)->prepares;
                auto as = current->active_schedule.producers;
                flat_map<uint32_t, uint32_t> prepare_count;
                for (const auto &pre: prepares) {
                    if (prepare_count.find(pre.view) == prepare_count.end()) prepare_count[pre.view] = 0;
                }

                if (!(*curr_itr)->should_prepared) {
                    for (auto const &sp: as) {
                        for (auto const &pp: prepares) {
                            if (sp.block_signing_key == pp.public_key) prepare_count[pp.view] += 1;
                        }
                    }
                    for (auto const &e: prepare_count) {
                        if (e.second >= as.size() * 2 / 3 + 1) {
                            by_block_id_index.modify(curr_itr,
                                                     [&](const pbft_state_ptr &psp) { psp->should_prepared = true; });
                        }
                    }
                }
                current = ctrl.fetch_block_state_by_id(current->prev());
            }
        }


        vector<pbft_prepare> pbft_database::send_and_add_pbft_prepare(const vector<pbft_prepare> &pv, uint32_t current_view) {

            auto head_block_num = ctrl.head_block_num();
            if (head_block_num <= 1) return vector<pbft_prepare>{};
            auto my_prepare = ctrl.get_pbft_my_prepare();

            auto reserve_prepare = [&](const block_id_type &in) {
                if (in == block_id_type{} || !ctrl.fetch_block_state_by_id(in)) return false;
                auto lib = ctrl.last_irreversible_block_id();
                if (lib == block_id_type{}) return true;
                auto forks = ctrl.fork_db().fetch_branch_from(in, lib);
                return !forks.first.empty() && forks.second.empty();
            };

            vector<pbft_prepare> new_pv;
            if (!pv.empty()) {
                for (auto p : pv) {
                    //change uuid, sign again, update cache, then emit
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    p.uuid = uuid;
                    p.timestamp = time_point::now();
                    p.producer_signature = ctrl.my_signature_providers()[p.public_key](p.digest());
                    emit(pbft_outgoing_prepare, p);
                }
                return vector<pbft_prepare>{};
            } else if (reserve_prepare(my_prepare)) {
                for (auto const &sp : ctrl.my_signature_providers()) {
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    auto my_prepare_num = ctrl.fetch_block_state_by_id(my_prepare)->block_num;
                    auto p = pbft_prepare{uuid, current_view, my_prepare_num, my_prepare, sp.first, chain_id()};
                    p.producer_signature = sp.second(p.digest());
                    emit(pbft_outgoing_prepare, p);
                    new_pv.emplace_back(p);
                }
                return new_pv;
            } else {
                uint32_t high_water_mark_block_num = head_block_num;
                auto next_proposed_schedule_block_num = ctrl.get_global_properties().proposed_schedule_block_num;
                auto promoted_proposed_schedule_block_num = ctrl.last_promoted_proposed_schedule_block_num();
                auto lib = ctrl.last_irreversible_block_num();

                if (next_proposed_schedule_block_num && *next_proposed_schedule_block_num > lib) {
                    high_water_mark_block_num = std::min(head_block_num, *next_proposed_schedule_block_num);
                }

                if (promoted_proposed_schedule_block_num && promoted_proposed_schedule_block_num > lib) {
                    high_water_mark_block_num = std::min(high_water_mark_block_num,
                                                         promoted_proposed_schedule_block_num);
                }

                if (high_water_mark_block_num <= lib) return vector<pbft_prepare>{};
                block_id_type high_water_mark_block_id = ctrl.get_block_id_for_num(high_water_mark_block_num);
                for (auto const &sp : ctrl.my_signature_providers()) {
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    auto p = pbft_prepare{uuid, current_view, high_water_mark_block_num, high_water_mark_block_id,
                                          sp.first, chain_id()};
                    p.producer_signature = sp.second(p.digest());
                    add_pbft_prepare(p);
                    emit(pbft_outgoing_prepare, p);
                    new_pv.emplace_back(p);
                    ctrl.set_pbft_my_prepare(high_water_mark_block_id);
                }
                return new_pv;
            }
        }

        bool pbft_database::should_prepared() {

            const auto &by_prepare_and_num_index = pbft_state_index.get<by_prepare_and_num>();
            auto itr = by_prepare_and_num_index.begin();
            if (itr == by_prepare_and_num_index.end()) return false;

            pbft_state_ptr psp = *itr;

            if (psp->should_prepared && (psp->block_num > ctrl.last_irreversible_block_num())) {
                ctrl.set_pbft_prepared((*itr)->block_id);
                return true;
            }
            return false;
        }

        bool pbft_database::is_valid_prepare(const pbft_prepare &p) {
            if (p.chain_id != chain_id()) return false;
            // a prepare msg under lscb (which is no longer in fork_db), can be treated as null, thus true.
            if (p.block_num <= ctrl.last_stable_checkpoint_block_num()) return true;
            if (!p.is_signature_valid()) return false;
            return should_recv_pbft_msg(p.public_key);
        }

        void pbft_database::add_pbft_commit(pbft_commit &c) {

            if (!is_valid_commit(c)) return;
            auto &by_block_id_index = pbft_state_index.get<by_block_id>();

            auto current = ctrl.fetch_block_state_by_id(c.block_id);

            while ((current) && (current->block_num > ctrl.last_irreversible_block_num())) {

                auto curr_itr = by_block_id_index.find(current->id);

                if (curr_itr == by_block_id_index.end()) {
                    try {
                        auto curr_ps = pbft_state{current->id, current->block_num, .commits={c}};
                        auto curr_psp = make_shared<pbft_state>(curr_ps);
                        pbft_state_index.insert(curr_psp);
                    } catch (...) {
                        EOS_ASSERT(false, pbft_exception, "commit insert failure: ${c}", ("c", c));
                    }
                } else {
                    auto commits = (*curr_itr)->commits;
                    auto p_itr = find_if(commits.begin(), commits.end(),
                                         [&](const pbft_commit &comm) {
                                             return comm.public_key == c.public_key && comm.view == c.view;
                                         });
                    if (p_itr == commits.end()) {
                        by_block_id_index.modify(curr_itr, [&](const pbft_state_ptr &psp) {
                            psp->commits.emplace_back(c);
                            std::sort(psp->commits.begin(), psp->commits.end(), less<>());
                        });
                    }
                }

                curr_itr = by_block_id_index.find(current->id);
                if (curr_itr == by_block_id_index.end()) return;

                auto commits = (*curr_itr)->commits;

                auto as = current->active_schedule;
                flat_map<uint32_t, uint32_t> commit_count;
                for (const auto &com: commits) {
                    if (commit_count.find(com.view) == commit_count.end()) commit_count[com.view] = 0;
                }

                if (!(*curr_itr)->should_committed) {

                    for (auto const &sp: as.producers) {
                        for (auto const &pc: commits) {
                            if (sp.block_signing_key == pc.public_key) commit_count[pc.view] += 1;
                        }
                    }

                    for (auto const &e: commit_count) {
                        if (e.second >= current->active_schedule.producers.size() * 2 / 3 + 1) {
                            by_block_id_index.modify(curr_itr,
                                                     [&](const pbft_state_ptr &psp) { psp->should_committed = true; });
                        }
                    }
                }
                current = ctrl.fetch_block_state_by_id(current->prev());
            }
        }

        vector<pbft_commit> pbft_database::send_and_add_pbft_commit(const vector<pbft_commit> &cv, uint32_t current_view) {
            if (!cv.empty()) {
                for (auto c : cv) {
                    //change uuid, sign again, update cache, then emit
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    c.uuid = uuid;
                    c.timestamp = time_point::now();
                    c.producer_signature = ctrl.my_signature_providers()[c.public_key](c.digest());
                    emit(pbft_outgoing_commit, c);
                }
                return vector<pbft_commit>{};
            } else {
                const auto &by_prepare_and_num_index = pbft_state_index.get<by_prepare_and_num>();
                auto itr = by_prepare_and_num_index.begin();
                if (itr == by_prepare_and_num_index.end()) {
                    return vector<pbft_commit>{};
                }
                vector<pbft_commit> new_cv;
                pbft_state_ptr psp = *itr;
                auto bs = ctrl.fork_db().get_block(psp->block_id);

                if (psp->should_prepared && (psp->block_num > ctrl.last_irreversible_block_num())) {

                    for (auto const &sp : ctrl.my_signature_providers()) {
                        auto uuid = boost::uuids::to_string(uuid_generator());
                        auto c = pbft_commit{uuid, current_view, psp->block_num, psp->block_id, sp.first, chain_id()};
                        c.producer_signature = sp.second(c.digest());
                        add_pbft_commit(c);
                        emit(pbft_outgoing_commit, c);
                        new_cv.emplace_back(c);
                    }
                }
                return new_cv;
            }
        }

        bool pbft_database::should_committed() {
            const auto &by_commit_and_num_index = pbft_state_index.get<by_commit_and_num>();
            auto itr = by_commit_and_num_index.begin();
            if (itr == by_commit_and_num_index.end()) return false;
            pbft_state_ptr psp = *itr;

            return (psp->should_committed && (psp->block_num > ctrl.last_irreversible_block_num()));
        }

        uint32_t pbft_database::get_committed_view() {
            uint32_t new_view = 0;
            if (!should_committed()) return new_view;

            const auto &by_commit_and_num_index = pbft_state_index.get<by_commit_and_num>();
            auto itr = by_commit_and_num_index.begin();
            pbft_state_ptr psp = *itr;

            auto blk_state = ctrl.fetch_block_state_by_id((*itr)->block_id);
            if (!blk_state) return new_view;
            auto as = blk_state->active_schedule.producers;

            auto commits = (*itr)->commits;

            flat_map<uint32_t, uint32_t> commit_count;
            for (const auto &com: commits) {
                if (commit_count.find(com.view) == commit_count.end()) {
                    commit_count[com.view] = 1;
                } else {
                    commit_count[com.view] += 1;
                }
            }

            for (auto const &e: commit_count) {
                if (e.second >= as.size() * 2 / 3 + 1 && e.first > new_view) {
                    new_view = e.first;
                }
            }
            return new_view;
        }

        bool pbft_database::is_valid_commit(const pbft_commit &c) {
            if (c.chain_id != chain_id()) return false;
            if (c.block_num <= ctrl.last_stable_checkpoint_block_num()) return true;
            if (!c.is_signature_valid()) return false;
            return should_recv_pbft_msg(c.public_key);
        }

        void pbft_database::commit_local() {
            const auto &by_commit_and_num_index = pbft_state_index.get<by_commit_and_num>();
            auto itr = by_commit_and_num_index.begin();
            if (itr == by_commit_and_num_index.end()) return;

            pbft_state_ptr psp = *itr;

            ctrl.pbft_commit_local(psp->block_id);
        }

        bool pbft_database::pending_pbft_lib() {
            return ctrl.pending_pbft_lib();
        }

        void pbft_database::add_pbft_view_change(pbft_view_change &vc) {
            if (!is_valid_view_change(vc)) return;
            auto active_bps = lib_active_producers().producers;

            auto &by_view_index = view_state_index.get<by_view>();
            auto itr = by_view_index.find(vc.target_view);
            if (itr == by_view_index.end()) {
                auto vs = pbft_view_state{vc.target_view, .view_changes={vc}};
                auto vsp = make_shared<pbft_view_state>(vs);
                view_state_index.insert(vsp);
            } else {
                auto pvs = (*itr);
                auto view_changes = pvs->view_changes;
                auto p_itr = find_if(view_changes.begin(), view_changes.end(),
                                     [&](const pbft_view_change &existed) {
                                         return existed.public_key == vc.public_key;
                                     });
                if (p_itr == view_changes.end()) {
                    by_view_index.modify(itr, [&](const pbft_view_state_ptr &pvsp) {
                        pvsp->view_changes.emplace_back(vc);
                    });
                }
            }

            itr = by_view_index.find(vc.target_view);
            if (itr == by_view_index.end()) return;

            auto vc_count = 0;
            if (!(*itr)->should_view_changed) {
                for (auto const &sp: active_bps) {
                    for (auto const &v: (*itr)->view_changes) {
                        if (sp.block_signing_key == v.public_key) vc_count += 1;
                    }
                }
                if (vc_count >= active_bps.size() * 2 / 3 + 1) {
                    by_view_index.modify(itr, [&](const pbft_view_state_ptr &pvsp) { pvsp->should_view_changed = true; });
                }
            }
        }

        uint32_t pbft_database::should_view_change() {
            uint32_t nv = 0;
            auto &by_view_index = view_state_index.get<by_view>();
            auto itr = by_view_index.begin();
            if (itr == by_view_index.end()) return nv;

            while (itr != by_view_index.end()) {
                auto active_bps = lib_active_producers().producers;
                auto vc_count = 0;
                auto pvs = (*itr);

                for (auto const &bp: active_bps) {
                    for (auto const &pp: pvs->view_changes) {
                        if (bp.block_signing_key == pp.public_key) vc_count += 1;
                    }
                }
                //if contains self or view_change >= f+1, transit to view_change and send view change
                if (vc_count >= active_bps.size() / 3 + 1) {
                    nv = pvs->view;
                    break;
                }
                ++itr;
            }
            return nv;
        }

        vector<pbft_view_change> pbft_database::send_and_add_pbft_view_change(
                const vector<pbft_view_change> &vcv,
                const vector<pbft_prepared_certificate> &ppc,
                uint32_t current_view,
                uint32_t new_view) {
            if (!vcv.empty()) {
                for (auto vc : vcv) {
                    //change uuid, sign again, update cache, then emit
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    vc.uuid = uuid;
                    vc.timestamp = time_point::now();
                    vc.producer_signature = ctrl.my_signature_providers()[vc.public_key](vc.digest());
                    emit(pbft_outgoing_view_change, vc);
                }
                return vector<pbft_view_change>{};
            } else {
                vector<pbft_view_change> new_vcv;

                for (auto const &my_sp : ctrl.my_signature_providers()) {
                    auto ppc_ptr = find_if(ppc.begin(), ppc.end(),
                                           [&](const pbft_prepared_certificate &v) {
                                               return v.public_key == my_sp.first;
                                           });

                    auto my_ppc = pbft_prepared_certificate{};
                    if (ppc_ptr != ppc.end()) my_ppc = *ppc_ptr;
                    auto my_lsc = get_stable_checkpoint_by_id(ctrl.last_stable_checkpoint_block_id());
                    auto uuid = boost::uuids::to_string(uuid_generator());
                    auto vc = pbft_view_change{uuid, current_view, new_view, my_ppc, my_lsc, my_sp.first, chain_id()};
                    vc.producer_signature = my_sp.second(vc.digest());
                    emit(pbft_outgoing_view_change, vc);
                    add_pbft_view_change(vc);
                    new_vcv.emplace_back(vc);
                }
                return new_vcv;
            }
        }

        bool pbft_database::should_new_view(const uint32_t target_view) {
            auto &by_view_index = view_state_index.get<by_view>();
            auto itr = by_view_index.find(target_view);
            if (itr == by_view_index.end()) return false;
            return (*itr)->should_view_changed;
        }

        uint32_t pbft_database::get_proposed_new_view_num() {
            auto &by_count_and_view_index = view_state_index.get<by_count_and_view>();
            auto itr = by_count_and_view_index.begin();
            if (itr == by_count_and_view_index.end() || !(*itr)->should_view_changed) return 0;
            return (*itr)->view;
        }

        bool pbft_database::is_new_primary(const uint32_t target_view) {

            auto primary_key = get_new_view_primary_key(target_view);
            if (primary_key == public_key_type{}) return false;
            auto sps = ctrl.my_signature_providers();
            auto sp_itr = sps.find(primary_key);
            return sp_itr != sps.end();
        }

        void pbft_database::prune_pbft_index() {
            pbft_state_index.clear();
            view_state_index.clear();
            ctrl.reset_pbft_my_prepare();
        }

        pbft_new_view pbft_database::send_pbft_new_view(
                const vector<pbft_view_changed_certificate> &vcc,
                uint32_t current_view) {

            auto primary_key = get_new_view_primary_key(current_view);
            if (!is_new_primary(current_view)) return pbft_new_view{};

            //`sp_itr` is not possible to be the end iterator, since it's already been checked in `is_new_primary`.
            auto my_sps = ctrl.my_signature_providers();
            auto sp_itr = my_sps.find(primary_key);

            auto vcc_ptr = find_if(vcc.begin(), vcc.end(),
                                   [&](const pbft_view_changed_certificate &v) { return v.public_key == primary_key; });

            if (vcc_ptr == vcc.end()) return pbft_new_view{};

            auto highest_ppc = pbft_prepared_certificate{};
            auto highest_sc = pbft_stable_checkpoint{};

            for (const auto &vc: vcc_ptr->view_changes) {
                if (vc.prepared.block_num > highest_ppc.block_num && is_valid_prepared_certificate(vc.prepared)) {
                    highest_ppc = vc.prepared;
                }

                if (vc.stable_checkpoint.block_num > highest_sc.block_num &&
                    is_valid_stable_checkpoint(vc.stable_checkpoint)) {
                    highest_sc = vc.stable_checkpoint;
                }
            }

            auto uuid = boost::uuids::to_string(uuid_generator());
            auto nv = pbft_new_view{uuid, current_view, highest_ppc, highest_sc, *vcc_ptr, sp_itr->first, chain_id()};
            nv.producer_signature = sp_itr->second(nv.digest());
            emit(pbft_outgoing_new_view, nv);
            return nv;
        }

        vector<pbft_prepared_certificate> pbft_database::generate_prepared_certificate() {
            auto ppc = vector<pbft_prepared_certificate>{};
            const auto &by_prepare_and_num_index = pbft_state_index.get<by_prepare_and_num>();
            auto itr = by_prepare_and_num_index.begin();
            if (itr == by_prepare_and_num_index.end()) return vector<pbft_prepared_certificate>{};
            pbft_state_ptr psp = *itr;

            auto prepared_block_state = ctrl.fetch_block_state_by_id(psp->block_id);
            if (!prepared_block_state) return vector<pbft_prepared_certificate>{};

            auto as = prepared_block_state->active_schedule.producers;
            if (psp->should_prepared && (psp->block_num > (ctrl.last_irreversible_block_num()))) {
                for (auto const &my_sp : ctrl.my_signature_providers()) {
                    auto prepares = psp->prepares;
                    auto valid_prepares = vector<pbft_prepare>{};

                    flat_map<uint32_t, uint32_t> prepare_count;
                    flat_map<uint32_t, vector<pbft_prepare>> prepare_msg;

                    for (const auto &pre: prepares) {
                        if (prepare_count.find(pre.view) == prepare_count.end()) prepare_count[pre.view] = 0;
                        prepare_msg[pre.view].push_back(pre);
                    }

                    for (auto const &sp: as) {
                        for (auto const &pp: prepares) {
                            if (sp.block_signing_key == pp.public_key) prepare_count[pp.view] += 1;
                        }
                    }

                    for (auto const &e: prepare_count) {
                        if (e.second >= as.size() * 2 / 3 + 1) {
                            valid_prepares = prepare_msg[e.first];
                        }
                    }

                    if (valid_prepares.empty()) return vector<pbft_prepared_certificate>{};

                    auto pc = pbft_prepared_certificate{psp->block_id, psp->block_num, valid_prepares, my_sp.first};
                    pc.producer_signature = my_sp.second(pc.digest());
                    ppc.emplace_back(pc);
                }
                return ppc;
            } else return vector<pbft_prepared_certificate>{};
        }

        vector<pbft_view_changed_certificate> pbft_database::generate_view_changed_certificate(uint32_t target_view) {
            auto vcc = vector<pbft_view_changed_certificate>{};

            auto &by_view_index = view_state_index.get<by_view>();
            auto itr = by_view_index.find(target_view);
            if (itr == by_view_index.end()) return vcc;

            auto pvs = *itr;

            if (pvs->should_view_changed) {
                for (auto const &my_sp : ctrl.my_signature_providers()) {
                    auto pc = pbft_view_changed_certificate{pvs->view, pvs->view_changes, my_sp.first};
                    pc.producer_signature = my_sp.second(pc.digest());
                    vcc.emplace_back(pc);
                }
                return vcc;
            } else return vector<pbft_view_changed_certificate>{};
        }

        bool pbft_database::is_valid_prepared_certificate(const eosio::chain::pbft_prepared_certificate &certificate) {
            // an empty certificate is valid since it acts as a null digest in pbft.
            if (certificate == pbft_prepared_certificate{}) return true;
            // a certificate under lscb (no longer in fork_db) is also treated as null.
            if (certificate.block_num <= ctrl.last_stable_checkpoint_block_num()) return true;

            auto valid = true;
            valid &= certificate.is_signature_valid();
            for (auto const &p : certificate.prepares) {
                valid &= is_valid_prepare(p);
                if (!valid) return false;
            }

            auto cert_id = certificate.block_id;
            auto cert_bs = ctrl.fetch_block_state_by_id(cert_id);
            auto producer_schedule = lib_active_producers();
            if (certificate.block_num > 0 && cert_bs) {
                producer_schedule = cert_bs->active_schedule;
            }
            auto bp_threshold = producer_schedule.producers.size() * 2 / 3 + 1;

            auto prepares = certificate.prepares;
            flat_map<uint32_t, uint32_t> prepare_count;

            for (const auto &pre: prepares) {
                if (prepare_count.find(pre.view) == prepare_count.end()) prepare_count[pre.view] = 0;
            }

            for (auto const &sp: producer_schedule.producers) {
                for (auto const &pp: prepares) {
                    if (sp.block_signing_key == pp.public_key) prepare_count[pp.view] += 1;
                }
            }

            auto should_prepared = false;

            for (auto const &e: prepare_count) {
                if (e.second >= bp_threshold) {
                    should_prepared = true;
                }
            }

            if (!should_prepared) return false;

            {
                //validate prepare
                auto lscb = ctrl.last_stable_checkpoint_block_num();
                auto non_fork_bp_count = 0;
                vector<block_info> prepare_infos(certificate.prepares.size());
                for (auto const &p : certificate.prepares) {
                    //only search in fork db
                    if (p.block_num <= lscb) {
                        ++non_fork_bp_count;
                    } else {
                        prepare_infos.push_back(block_info{p.block_id, p.block_num});
                    }
                }

                auto prepare_forks = fetch_fork_from(prepare_infos);
                vector<block_info> longest_fork;
                for (auto const &f : prepare_forks) {
                    if (f.size() > longest_fork.size()) {
                        longest_fork = f;
                    }
                }
                if (longest_fork.size() + non_fork_bp_count < bp_threshold) return false;

                if (longest_fork.empty()) return true;

                auto calculated_block_info = longest_fork.back();

                auto current_bs = ctrl.fetch_block_state_by_id(calculated_block_info.block_id);
                while (current_bs) {
                    if (certificate.block_id == current_bs->id && certificate.block_num == current_bs->block_num) {
                        return true;
                    }
                    current_bs = ctrl.fetch_block_state_by_id(current_bs->prev());
                }
                return false;
            }
        }

        bool pbft_database::is_valid_view_change(const pbft_view_change &vc) {
            if (vc.chain_id != chain_id()) return false;

            return vc.is_signature_valid()
                   && should_recv_pbft_msg(vc.public_key);
            // No need to check prepared cert and stable checkpoint, until generate or validate a new view msg
        }


        bool pbft_database::is_valid_new_view(const pbft_new_view &nv) {
            //all signatures should be valid
            if (nv.chain_id != chain_id()) return false;

            auto valid = is_valid_prepared_certificate(nv.prepared)
                         && is_valid_stable_checkpoint(nv.stable_checkpoint)
                         && nv.view_changed.is_signature_valid()
                         && nv.is_signature_valid();
            if (!valid) return false;
            if (nv.view_changed.view != nv.view) return false;
            auto schedule_threshold = lib_active_producers().producers.size() * 2 / 3 + 1;

            if (nv.view_changed.view_changes.size() < schedule_threshold) return false;
            for (auto vc: nv.view_changed.view_changes) {
                if (!is_valid_view_change(vc)) return false;
                add_pbft_view_change(vc);
            }

            if (!should_new_view(nv.view)) return false;

            auto highest_ppc = pbft_prepared_certificate{};
            auto highest_scp = pbft_stable_checkpoint{};

            for (const auto &vc: nv.view_changed.view_changes) {
                if (vc.prepared.block_num > highest_ppc.block_num
                    && is_valid_prepared_certificate(vc.prepared)) {
                    highest_ppc = vc.prepared;
                }

                if (vc.stable_checkpoint.block_num > highest_scp.block_num
                    && is_valid_stable_checkpoint(vc.stable_checkpoint)) {
                    highest_scp = vc.stable_checkpoint;
                }
            }

            return highest_ppc == nv.prepared
                   && highest_scp == nv.stable_checkpoint;
        }

        bool pbft_database::should_stop_view_change(const pbft_view_change &vc) {
            auto lscb_num = ctrl.last_stable_checkpoint_block_num();
            return lscb_num > vc.prepared.block_num
                   && lscb_num > vc.stable_checkpoint.block_num;
        }

        vector<vector<block_info>> pbft_database::fetch_fork_from(const vector<block_info> block_infos) {
            auto bi = block_infos;

            vector<vector<block_info>> result;
            if (bi.empty()) {
                return result;
            }
            if (bi.size() == 1) {
                result.emplace_back(initializer_list<block_info>{bi.front()});
                return result;
            }

            sort(bi.begin(), bi.end(),
                 [](const block_info &a, const block_info &b) -> bool { return a.block_num > b.block_num; });

            while (!bi.empty()) {
                auto fork = fetch_first_fork_from(bi);
                if (!fork.empty()) {
                    result.emplace_back(fork);
                }
            }
            return result;
        }

        vector<block_info> pbft_database::fetch_first_fork_from(vector<block_info> &bi) {
            vector<block_info> result;
            if (bi.empty()) {
                return result;
            }
            if (bi.size() == 1) {
                result.emplace_back(bi.front());
                bi.clear();
                return result;
            }
            //bi should be sorted desc
            auto high = bi.front().block_num;
            auto low = bi.back().block_num;

            auto id = bi.front().block_id;
            auto num = bi.front().block_num;
            while (num <= high && num >= low && !bi.empty()) {
                auto bs = ctrl.fetch_block_state_by_id(id);

                for (auto it = bi.begin(); it != bi.end();) {
                    if (it->block_id == id) {
                        if (bs) {
                            //add to result only if b exist
                            result.emplace_back((*it));
                        }
                        it = bi.erase(it);
                    } else {
                        it++;
                    }
                }
                if (bs) {
                    id = bs->prev();
                    num--;
                } else {
                    break;
                }
            }

            return result;
        }

        pbft_stable_checkpoint pbft_database::fetch_stable_checkpoint_from_blk_extn(const signed_block_ptr &b) {
            try {
                auto &ext = b->block_extensions;

                for (auto it = ext.begin(); it != ext.end();) {
                    if (it->first == static_cast<uint16_t>(block_extension_type::pbft_stable_checkpoint))
                    {
                        auto scp_v = it->second;
                        fc::datastream<char *> ds_decode(scp_v.data(), scp_v.size());

                        pbft_stable_checkpoint scp_decode;
                        fc::raw::unpack(ds_decode, scp_decode);

                        if (is_valid_stable_checkpoint(scp_decode)) {
                            return scp_decode;
                        } else {
                            it = ext.erase(it);
                        }
                    } else {
                        it++;
                    }
                }
            } catch(...) {
                wlog("no stable checkpoints found in the block extension");
            }
            return pbft_stable_checkpoint{};
        }

        pbft_stable_checkpoint pbft_database::get_stable_checkpoint_by_id(const block_id_type &block_id) {
            const auto &by_block = checkpoint_index.get<by_block_id>();
            auto itr = by_block.find(block_id);
            if (itr == by_block.end()) {
                auto blk = ctrl.fetch_block_by_id(block_id);
                return fetch_stable_checkpoint_from_blk_extn(blk);
            }

            auto cpp = *itr;

            if (cpp->is_stable) {
                if (ctrl.my_signature_providers().empty()) return pbft_stable_checkpoint{};
                auto psc = pbft_stable_checkpoint{cpp->block_num, cpp->block_id, cpp->checkpoints, chain_id()};
                return psc;
            } else return pbft_stable_checkpoint{};
        }

        block_info pbft_database::cal_pending_stable_checkpoint() const {

            auto lscb_num = ctrl.last_stable_checkpoint_block_num();
            auto lscb_id = ctrl.last_stable_checkpoint_block_id();
            auto lscb_info = block_info{lscb_id, lscb_num};

            const auto &by_blk_num = checkpoint_index.get<by_num>();
            auto itr = by_blk_num.lower_bound(lscb_num);
            if (itr == by_blk_num.end()) return lscb_info;

            while (itr != by_blk_num.end()) {
                if ((*itr)->is_stable && ctrl.fetch_block_state_by_id((*itr)->block_id)) {
                    auto lib = ctrl.fetch_block_state_by_number(ctrl.last_irreversible_block_num());

                    auto head_checkpoint_schedule = ctrl.fetch_block_state_by_id(
                            (*itr)->block_id)->active_schedule;

                    auto current_schedule = lib_active_producers();
                    auto new_schedule = lib_active_producers();

                    if (lib) {
                        current_schedule = lib->active_schedule;
                        new_schedule = lib->pending_schedule;
                    }

                    if ((*itr)->is_stable
                        && (head_checkpoint_schedule == current_schedule || head_checkpoint_schedule == new_schedule)) {
                        lscb_info.block_id = (*itr)->block_id;
                        lscb_info.block_num = (*itr)->block_num;
                    }
                }
                ++itr;
            }
            return lscb_info;
        }

        vector<pbft_checkpoint> pbft_database::generate_and_add_pbft_checkpoint() {
            auto new_pc = vector<pbft_checkpoint>{};

            const auto &by_commit_and_num_index = pbft_state_index.get<by_commit_and_num>();
            auto itr = by_commit_and_num_index.begin();
            if (itr == by_commit_and_num_index.end() || !(*itr)->should_committed) return new_pc;

            pbft_state_ptr psp = (*itr);

            vector<block_num_type> pending_checkpoint_block_num;

            block_num_type my_latest_checkpoint = 0;

            auto checkpoint = [&](const block_num_type &in) {
                return in % 100 == 1
                       || in == ctrl.last_proposed_schedule_block_num()
                       || in == ctrl.last_promoted_proposed_schedule_block_num();
            };

            for (auto i = psp->block_num;
                 i > std::max(ctrl.last_stable_checkpoint_block_num(), static_cast<uint32_t>(1)); --i) {
                if (checkpoint(i)) {
                    my_latest_checkpoint = max(i, my_latest_checkpoint);
                    auto &by_block = checkpoint_index.get<by_block_id>();
                    auto c_itr = by_block.find(ctrl.get_block_id_for_num(i));
                    if (c_itr == by_block.end()) {
                        pending_checkpoint_block_num.emplace_back(i);
                    } else {
                        auto checkpoints = (*c_itr)->checkpoints;
                        bool contains_mine = false;
                        for (auto const &my_sp : ctrl.my_signature_providers()) {
                            auto p_itr = find_if(checkpoints.begin(), checkpoints.end(),
                                                 [&](const pbft_checkpoint &ext) {
                                                     return ext.public_key == my_sp.first;
                                                 });
                            if (p_itr != checkpoints.end()) contains_mine = true;
                        }
                        if (!contains_mine) {
                            pending_checkpoint_block_num.emplace_back(i);
                        }
                    }
                }
            }

            if (!pending_checkpoint_block_num.empty()) {
                for (auto h: pending_checkpoint_block_num) {
                    for (auto const &my_sp : ctrl.my_signature_providers()) {
                        auto uuid = boost::uuids::to_string(uuid_generator());
                        auto cp = pbft_checkpoint{uuid, h, ctrl.get_block_id_for_num(h),
                                                  my_sp.first, .chain_id=chain_id()};
                        cp.producer_signature = my_sp.second(cp.digest());
                        add_pbft_checkpoint(cp);
                        new_pc.emplace_back(cp);
                    }
                }
            } else if (my_latest_checkpoint > 1) {
                auto lscb_id = ctrl.get_block_id_for_num(my_latest_checkpoint);
                auto &by_block = checkpoint_index.get<by_block_id>();
                auto h_itr = by_block.find(lscb_id);
                if (h_itr != by_block.end()) {
                    auto checkpoints = (*h_itr)->checkpoints;
                    for (auto const &my_sp : ctrl.my_signature_providers()) {
                        for (auto const &cp: checkpoints) {
                            if (my_sp.first == cp.public_key) {
                                auto retry_cp = cp;
                                auto uuid = boost::uuids::to_string(uuid_generator());
                                retry_cp.uuid = uuid;
                                retry_cp.timestamp = time_point::now();
                                retry_cp.producer_signature = my_sp.second(retry_cp.digest());
                                new_pc.emplace_back(retry_cp);
                            }
                        }
                    }
                }
            }

            return new_pc;
        }

        void pbft_database::add_pbft_checkpoint(pbft_checkpoint &cp) {

            if (!is_valid_checkpoint(cp)) return;

            auto lscb_num = ctrl.last_stable_checkpoint_block_num();

            auto cp_block_state = ctrl.fetch_block_state_by_id(cp.block_id);
            if (!cp_block_state) return;
            auto active_bps = cp_block_state->active_schedule.producers;
            auto checkpoint_count = count_if(active_bps.begin(), active_bps.end(), [&](const producer_key &p) {
                return p.block_signing_key == cp.public_key;
            });
            if (checkpoint_count == 0) return;

            auto &by_block = checkpoint_index.get<by_block_id>();
            auto itr = by_block.find(cp.block_id);
            if (itr == by_block.end()) {
                auto cs = pbft_checkpoint_state{cp.block_id, cp.block_num, .checkpoints={cp}};
                auto csp = make_shared<pbft_checkpoint_state>(cs);
                checkpoint_index.insert(csp);
                itr = by_block.find(cp.block_id);
            } else {
                auto csp = (*itr);
                auto checkpoints = csp->checkpoints;
                auto p_itr = find_if(checkpoints.begin(), checkpoints.end(),
                                     [&](const pbft_checkpoint &existed) {
                                         return existed.public_key == cp.public_key;
                                     });
                if (p_itr == checkpoints.end()) {
                    by_block.modify(itr, [&](const pbft_checkpoint_state_ptr &pcp) {
                        csp->checkpoints.emplace_back(cp);
                    });
                }
            }

            auto csp = (*itr);
            auto cp_count = 0;
            if (!csp->is_stable) {
                for (auto const &sp: active_bps) {
                    for (auto const &pp: csp->checkpoints) {
                        if (sp.block_signing_key == pp.public_key) cp_count += 1;
                    }
                }
                if (cp_count >= active_bps.size() * 2 / 3 + 1) {
                    by_block.modify(itr, [&](const pbft_checkpoint_state_ptr &pcp) { csp->is_stable = true; });
                    auto id = csp->block_id;
                    auto blk = ctrl.fetch_block_by_id(id);

                    if (blk && (blk->block_extensions.empty() || blk->block_extensions.back().first != static_cast<uint16_t>(block_extension_type::pbft_stable_checkpoint))) {
                        auto scp = get_stable_checkpoint_by_id(id);
                        auto scp_size = fc::raw::pack_size(scp);

                        auto buffer = std::make_shared<vector<char>>(scp_size);
                        fc::datastream<char*> ds( buffer->data(), scp_size);
                        fc::raw::pack( ds, scp );

                        blk->block_extensions.emplace_back();
                        auto &extension = blk->block_extensions.back();
                        extension.first = static_cast<uint16_t>(block_extension_type::pbft_stable_checkpoint );
                        extension.second.resize(scp_size);
                        std::copy(buffer->begin(),buffer->end(), extension.second.data());
                    }
                }
            }

            auto lscb_info = cal_pending_stable_checkpoint();
            auto pending_num = lscb_info.block_num;
            auto pending_id = lscb_info.block_id;
            if (pending_num > lscb_num) {
                ctrl.set_pbft_latest_checkpoint(pending_id);
                if (ctrl.last_irreversible_block_num() < pending_num) ctrl.pbft_commit_local(pending_id);
                const auto &by_block_id_index = pbft_state_index.get<by_block_id>();
                auto pitr = by_block_id_index.find(pending_id);
                if (pitr != by_block_id_index.end()) {
                    prune(*pitr);
                }
            }

        }

        void pbft_database::send_pbft_checkpoint() {
            auto cps_to_send = generate_and_add_pbft_checkpoint();
            for (auto const &cp: cps_to_send) {
                emit(pbft_outgoing_checkpoint, cp);
            }
        }

        bool pbft_database::is_valid_checkpoint(const pbft_checkpoint &cp) {

            if (cp.block_num > ctrl.head_block_num()
                || cp.block_num <= ctrl.last_stable_checkpoint_block_num()
                || !cp.is_signature_valid())
                return false;
            auto bs = ctrl.fetch_block_state_by_id(cp.block_id);
            if (bs) {
                auto active_bps = bs->active_schedule.producers;
                for (const auto &bp: active_bps) {
                    if (bp.block_signing_key == cp.public_key) return true;
                }
            }
            return false;
        }

        bool pbft_database::is_valid_stable_checkpoint(const pbft_stable_checkpoint &scp) {
            if (scp.block_num <= ctrl.last_stable_checkpoint_block_num()) return true;

            auto valid = true;
            for (const auto &c: scp.checkpoints) {
                valid &= is_valid_checkpoint(c)
                         && c.block_id == scp.block_id
                         && c.block_num == scp.block_num;
                if (!valid) return false;
            }
            //TODO: check if (2/3 + 1) met
            return valid;
        }

        bool pbft_database::should_send_pbft_msg() {

            //use last_stable_checkpoint producer schedule
            auto lscb_num = ctrl.last_stable_checkpoint_block_num();

            auto as = lib_active_producers();
            auto my_sp = ctrl.my_signature_providers();

            for (auto i = lscb_num; i <= ctrl.head_block_num(); ++i) {
                for (auto const &bp: as.producers) {
                    for (auto const &my: my_sp) {
                        if (bp.block_signing_key == my.first) return true;
                    }
                }
                auto bs = ctrl.fetch_block_state_by_number(i);
                if (bs && bs->active_schedule != as) as = bs->active_schedule;
            }
            return false;
        }

        bool pbft_database::should_recv_pbft_msg(const public_key_type &pub_key) {
            auto lscb_num = ctrl.last_stable_checkpoint_block_num();

            auto as = lib_active_producers();
            auto my_sp = ctrl.my_signature_providers();

            for (auto i = lscb_num; i <= ctrl.head_block_num(); ++i) {
                for (auto const &bp: as.producers) {
                    if (bp.block_signing_key == pub_key) return true;
                }
                auto bs = ctrl.fetch_block_state_by_number(i);
                if (bs && bs->active_schedule != as) as = bs->active_schedule;
            }
            return false;
        }

        public_key_type pbft_database::get_new_view_primary_key(const uint32_t target_view) {

            auto active_bps = lib_active_producers().producers;
            if (active_bps.empty()) return public_key_type{};

            return active_bps[target_view % active_bps.size()].block_signing_key;
        }

        producer_schedule_type pbft_database::lib_active_producers() const {
            auto lib_num = ctrl.last_irreversible_block_num();
            if (lib_num == 0) return ctrl.initial_schedule();

            auto lib_state = ctrl.fetch_block_state_by_number(lib_num);
            if (!lib_state) return ctrl.initial_schedule();

            if (lib_state->pending_schedule.producers.empty()) return lib_state->active_schedule;
            return lib_state->pending_schedule;
        }

        chain_id_type pbft_database::chain_id() {
            return ctrl.get_chain_id();
        }

        void pbft_database::set(pbft_state_ptr s) {
            auto result = pbft_state_index.insert(s);

            EOS_ASSERT(result.second, pbft_exception,
                       "unable to insert pbft state, duplicate state detected");
        }

        void pbft_database::set(pbft_checkpoint_state_ptr s) {
            auto result = checkpoint_index.insert(s);

            EOS_ASSERT(result.second, pbft_exception,
                       "unable to insert pbft checkpoint index, duplicate state detected");
        }

        void pbft_database::prune(const pbft_state_ptr &h) {
            auto num = h->block_num;

            auto &by_bn = pbft_state_index.get<by_num>();
            auto bni = by_bn.begin();
            while (bni != by_bn.end() && (*bni)->block_num < num) {
                prune(*bni);
                bni = by_bn.begin();
            }

            auto itr = pbft_state_index.find(h->block_id);
            if (itr != pbft_state_index.end()) {
                pbft_state_index.erase(itr);
            }
        }

        template<typename Signal, typename Arg>
        void pbft_database::emit(const Signal &s, Arg &&a) {
            try {
                s(std::forward<Arg>(a));
            } catch (boost::interprocess::bad_alloc &e) {
                wlog("bad alloc");
                throw e;
            } catch (controller_emit_signal_exception &e) {
                wlog("${details}", ("details", e.to_detail_string()));
                throw e;
            } catch (fc::exception &e) {
                wlog("${details}", ("details", e.to_detail_string()));
            } catch (...) {
                wlog("signal handler threw exception");
            }
        }
    }
}