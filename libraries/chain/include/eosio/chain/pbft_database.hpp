#pragma once

#include <eosio/chain/controller.hpp>
#include <eosio/chain/fork_database.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace eosio {
    namespace chain {
        using boost::multi_index_container;
        using namespace boost::multi_index;
        using namespace std;
        using boost::uuids::uuid;


        struct block_info {
            block_id_type block_id;
            block_num_type block_num = 0;
        };

        struct pbft_prepare {
            string uuid;
            uint32_t view;
            block_num_type block_num = 0;
            block_id_type block_id;
            public_key_type public_key;
            chain_id_type chain_id = chain_id_type("");
            signature_type producer_signature;
            time_point timestamp = time_point::now();


            bool operator==(const pbft_prepare &rhs) const {
                return view == rhs.view
                       && block_num == rhs.block_num
                       && block_id == rhs.block_id
                       && public_key == rhs.public_key
                       && chain_id == rhs.chain_id
                       && timestamp == rhs.timestamp;
            }

            bool operator!=(const pbft_prepare &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_prepare &rhs) const {
                if (block_num < rhs.block_num) {
                    return true;
                } else return block_num == rhs.block_num && view < rhs.view;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, view);
                fc::raw::pack(enc, block_num);
                fc::raw::pack(enc, block_id);
                fc::raw::pack(enc, public_key);
                fc::raw::pack(enc, chain_id);
                fc::raw::pack(enc, timestamp);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_commit {
            string uuid;
            uint32_t view;
            block_num_type block_num = 0;
            block_id_type block_id;
            public_key_type public_key;
            chain_id_type chain_id = chain_id_type("");
            signature_type producer_signature;
            time_point timestamp = time_point::now();


            bool operator==(const pbft_commit &rhs) const {
                return view == rhs.view
                       && block_num == rhs.block_num
                       && block_id == rhs.block_id
                       && public_key == rhs.public_key
                       && chain_id == rhs.chain_id
                       && timestamp == rhs.timestamp;
            }

            bool operator!=(const pbft_commit &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_commit &rhs) const {
                if (block_num < rhs.block_num) {
                    return true;
                } else return block_num == rhs.block_num && view < rhs.view;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, view);
                fc::raw::pack(enc, block_num);
                fc::raw::pack(enc, block_id);
                fc::raw::pack(enc, public_key);
                fc::raw::pack(enc, chain_id);
                fc::raw::pack(enc, timestamp);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_checkpoint {
            string uuid;
            block_num_type block_num = 0;
            block_id_type block_id;
            public_key_type public_key;
            chain_id_type chain_id = chain_id_type("");
            signature_type producer_signature;
            time_point timestamp = time_point::now();

            bool operator==(const pbft_checkpoint &rhs) const {
                return block_num == rhs.block_num
                       && block_id == rhs.block_id
                       && public_key == rhs.public_key
                       && chain_id == rhs.chain_id
                       && timestamp == rhs.timestamp;

            }

            bool operator!=(const pbft_checkpoint &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_checkpoint &rhs) const {
                return block_num < rhs.block_num;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, block_num);
                fc::raw::pack(enc, block_id);
                fc::raw::pack(enc, public_key);
                fc::raw::pack(enc, chain_id);
                fc::raw::pack(enc, timestamp);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_stable_checkpoint {
            block_num_type block_num = 0;
            block_id_type block_id;
            vector<pbft_checkpoint> checkpoints;
            chain_id_type chain_id = chain_id_type("");

            bool operator==(const pbft_stable_checkpoint &rhs) const {
                return block_id == rhs.block_id
                       && block_num == rhs.block_num
                       && checkpoints == rhs.checkpoints
                       && chain_id == rhs.chain_id;
            }

            bool operator!=(const pbft_stable_checkpoint &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_stable_checkpoint &rhs) const {
                return block_num < rhs.block_num;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, block_num);
                fc::raw::pack(enc, block_id);
                fc::raw::pack(enc, checkpoints);
                fc::raw::pack(enc, chain_id);
                return enc.result();
            }
        };

        struct pbft_prepared_certificate {
            block_id_type block_id;
            block_num_type block_num = 0;
            vector<pbft_prepare> prepares;

            public_key_type public_key;
            signature_type producer_signature;

            bool operator==(const pbft_prepared_certificate &rhs) const {
                return block_num == rhs.block_num
                       && block_id == rhs.block_id
                       && prepares == rhs.prepares
                       && public_key == rhs.public_key;
            }

            bool operator!=(const pbft_prepared_certificate &rhs) const {
                return !(*this == rhs);
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, block_id);
                fc::raw::pack(enc, block_num);
                fc::raw::pack(enc, prepares);
                fc::raw::pack(enc, public_key);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_view_change {
            string uuid;
            uint32_t current_view;
            uint32_t target_view;
            pbft_prepared_certificate prepared;
            pbft_stable_checkpoint stable_checkpoint;
            public_key_type public_key;
            chain_id_type chain_id = chain_id_type("");
            signature_type producer_signature;
            time_point timestamp = time_point::now();

            bool operator==(const pbft_view_change &rhs) const {
                return current_view == rhs.current_view
                       && target_view == rhs.target_view
                       && prepared == rhs.prepared
                       && stable_checkpoint == rhs.stable_checkpoint
                       && public_key == rhs.public_key
                       && chain_id == rhs.chain_id
                       && timestamp == rhs.timestamp;
            }

            bool operator!=(const pbft_view_change &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_view_change &rhs) const {
                return target_view < rhs.target_view;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, current_view);
                fc::raw::pack(enc, target_view);
                fc::raw::pack(enc, prepared);
                fc::raw::pack(enc, stable_checkpoint);
                fc::raw::pack(enc, public_key);
                fc::raw::pack(enc, chain_id);
                fc::raw::pack(enc, timestamp);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_view_changed_certificate {
            uint32_t view;
            vector<pbft_view_change> view_changes;

            public_key_type public_key;
            signature_type producer_signature;

            bool operator==(const pbft_view_changed_certificate &rhs) const {
                return view == rhs.view
                       && view_changes == rhs.view_changes
                       && public_key == rhs.public_key;
            }

            bool operator!=(const pbft_view_changed_certificate &rhs) const {
                return !(*this == rhs);
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, view);
                fc::raw::pack(enc, view_changes);
                fc::raw::pack(enc, public_key);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_new_view {
            string uuid;
            uint32_t view;
            pbft_prepared_certificate prepared;
            pbft_stable_checkpoint stable_checkpoint;
            pbft_view_changed_certificate view_changed;
            public_key_type public_key;
            chain_id_type chain_id = chain_id_type("");
            signature_type producer_signature;
            time_point timestamp = time_point::now();

            bool operator==(const pbft_new_view &rhs) const {
                return view == rhs.view
                       && prepared == rhs.prepared
                       && stable_checkpoint == rhs.stable_checkpoint
                       && view_changed == rhs.view_changed
                       && public_key == rhs.public_key
                       && chain_id == rhs.chain_id
                       && timestamp == rhs.timestamp;
            }

            bool operator!=(const pbft_new_view &rhs) const {
                return !(*this == rhs);
            }

            bool operator<(const pbft_new_view &rhs) const {
                return view < rhs.view;
            }

            digest_type digest() const {
                digest_type::encoder enc;
                fc::raw::pack(enc, view);
                fc::raw::pack(enc, prepared);
                fc::raw::pack(enc, stable_checkpoint);
                fc::raw::pack(enc, view_changed);
                fc::raw::pack(enc, public_key);
                fc::raw::pack(enc, chain_id);
                fc::raw::pack(enc, timestamp);
                return enc.result();
            }

            bool is_signature_valid() const {
                try {
                    auto pk = crypto::public_key(producer_signature, digest(), true);
                    return public_key == pk;
                } catch (fc::exception & /*e*/) {
                    return false;
                }
            }
        };

        struct pbft_state {
            block_id_type block_id;
            block_num_type block_num = 0;
            vector<pbft_prepare> prepares;
            bool should_prepared = false;
            vector<pbft_commit> commits;
            bool should_committed = false;
        };

        struct pbft_view_state {
            uint32_t view;
            vector<pbft_view_change> view_changes;
            bool should_view_changed = false;
        };

        struct pbft_checkpoint_state {
            block_id_type block_id;
            block_num_type block_num = 0;
            vector<pbft_checkpoint> checkpoints;
            bool is_stable = false;
        };

        using pbft_state_ptr = std::shared_ptr<pbft_state>;
        using pbft_view_state_ptr = std::shared_ptr<pbft_view_state>;
        using pbft_checkpoint_state_ptr = std::shared_ptr<pbft_checkpoint_state>;

        struct by_block_id;
        struct by_num;
        struct by_prepare_and_num;
        struct by_commit_and_num;
        typedef multi_index_container<
                pbft_state_ptr,
                indexed_by<
                        hashed_unique<
                                tag<by_block_id>,
                                member<pbft_state, block_id_type, &pbft_state::block_id>,
                                std::hash<block_id_type>
                        >,
                        ordered_non_unique<
                                tag<by_num>,
                                composite_key<
                                        pbft_state,
                                        member<pbft_state, uint32_t, &pbft_state::block_num>
                                >,
                                composite_key_compare<less<>>
                        >,
                        ordered_non_unique<
                                tag<by_prepare_and_num>,
                                composite_key<
                                        pbft_state,
                                        member<pbft_state, bool, &pbft_state::should_prepared>,
                                        member<pbft_state, uint32_t, &pbft_state::block_num>
                                >,
                                composite_key_compare<greater<>, greater<>>
                        >,
                        ordered_non_unique<
                                tag<by_commit_and_num>,
                                composite_key<
                                        pbft_state,
                                        member<pbft_state, bool, &pbft_state::should_committed>,
                                        member<pbft_state, uint32_t, &pbft_state::block_num>
                                >,
                                composite_key_compare<greater<>, greater<>>
                        >
                >
        >
                pbft_state_multi_index_type;

        struct by_view;
        struct by_count_and_view;
        typedef multi_index_container<
                pbft_view_state_ptr,
                indexed_by<
                        hashed_unique<
                                tag<by_view>,
                                member<pbft_view_state, uint32_t, &pbft_view_state::view>,
                                std::hash<uint32_t>
                        >,
                        ordered_non_unique<
                                tag<by_count_and_view>,
                                composite_key<
                                        pbft_view_state,
                                        member<pbft_view_state, bool, &pbft_view_state::should_view_changed>,
                                        member<pbft_view_state, uint32_t, &pbft_view_state::view>
                                >,
                                composite_key_compare<greater<>, greater<>>
                        >
                >
        >
                pbft_view_state_multi_index_type;

        struct by_block_id;
        struct by_num;
        typedef multi_index_container<
                pbft_checkpoint_state_ptr,
                indexed_by<
                        hashed_unique<
                                tag<by_block_id>,
                                member<pbft_checkpoint_state, block_id_type, &pbft_checkpoint_state::block_id>,
                                std::hash<block_id_type>
                        >,
                        ordered_non_unique<
                                tag<by_num>,
                                composite_key<
                                        pbft_checkpoint_state,
//                                        member<pbft_checkpoint_state, bool, &pbft_checkpoint_state::is_stable>,
                                        member<pbft_checkpoint_state, uint32_t, &pbft_checkpoint_state::block_num>
                                >,
                                composite_key_compare<less<>>
                        >
                >
        >
                pbft_checkpoint_state_multi_index_type;

        class pbft_database {
        public:
            explicit pbft_database(controller &ctrl);

            ~pbft_database();

            void close();

            bool should_prepared();

            bool should_committed();

            uint32_t should_view_change();

            bool should_new_view(uint32_t target_view);

            bool is_new_primary(uint32_t target_view);

            uint32_t get_proposed_new_view_num();

            void add_pbft_prepare(pbft_prepare &p);

            void add_pbft_commit(pbft_commit &c);

            void add_pbft_view_change(pbft_view_change &vc);

            void add_pbft_checkpoint(pbft_checkpoint &cp);

            vector<pbft_prepare> send_and_add_pbft_prepare(
                    const vector<pbft_prepare> &pv = vector<pbft_prepare>{},
                    uint32_t current_view = 0);

            vector<pbft_commit> send_and_add_pbft_commit(
                    const vector<pbft_commit> &cv = vector<pbft_commit>{},
                    uint32_t current_view = 0);

            vector<pbft_view_change> send_and_add_pbft_view_change(
                    const vector<pbft_view_change> &vcv = vector<pbft_view_change>{},
                    const vector<pbft_prepared_certificate> &ppc = vector<pbft_prepared_certificate>{},
                    uint32_t current_view = 0,
                    uint32_t new_view = 1);

            pbft_new_view send_pbft_new_view(
                    const vector<pbft_view_changed_certificate> &vcc = vector<pbft_view_changed_certificate>{},
                    uint32_t current_view = 1);

            vector<pbft_checkpoint> generate_and_add_pbft_checkpoint();

            bool is_valid_prepare(const pbft_prepare &p);

            bool is_valid_commit(const pbft_commit &c);

            void commit_local();

            bool pending_pbft_lib();

            void prune_pbft_index();

            uint32_t get_committed_view();

            chain_id_type chain_id();

            vector<pbft_prepared_certificate> generate_prepared_certificate();

            vector<pbft_view_changed_certificate> generate_view_changed_certificate(uint32_t target_view);

            pbft_stable_checkpoint get_stable_checkpoint_by_id(const block_id_type &block_id);

            pbft_stable_checkpoint fetch_stable_checkpoint_from_blk_extn(const signed_block_ptr &b);

            block_info cal_pending_stable_checkpoint() const;

            bool should_send_pbft_msg();

            bool should_recv_pbft_msg(const public_key_type &pub_key);

            void send_pbft_checkpoint();

            bool is_valid_checkpoint(const pbft_checkpoint &cp);

            bool is_valid_stable_checkpoint(const pbft_stable_checkpoint &scp);

            signal<void(const pbft_prepare &)> pbft_outgoing_prepare;
            signal<void(const pbft_prepare &)> pbft_incoming_prepare;

            signal<void(const pbft_commit &)> pbft_outgoing_commit;
            signal<void(const pbft_commit &)> pbft_incoming_commit;

            signal<void(const pbft_view_change &)> pbft_outgoing_view_change;
            signal<void(const pbft_view_change &)> pbft_incoming_view_change;

            signal<void(const pbft_new_view &)> pbft_outgoing_new_view;
            signal<void(const pbft_new_view &)> pbft_incoming_new_view;

            signal<void(const pbft_checkpoint &)> pbft_outgoing_checkpoint;
            signal<void(const pbft_checkpoint &)> pbft_incoming_checkpoint;

            bool is_valid_view_change(const pbft_view_change &vc);

            bool is_valid_new_view(const pbft_new_view &nv);

            bool should_stop_view_change(const pbft_view_change &vc);

        private:
            controller &ctrl;
            pbft_state_multi_index_type pbft_state_index;
            pbft_view_state_multi_index_type view_state_index;
            pbft_checkpoint_state_multi_index_type checkpoint_index;
            fc::path pbft_db_dir;
            fc::path checkpoints_dir;
            boost::uuids::random_generator uuid_generator;

            bool is_valid_prepared_certificate(const pbft_prepared_certificate &certificate);

            public_key_type get_new_view_primary_key(uint32_t target_view);

            vector<vector<block_info>> fetch_fork_from(vector<block_info> block_infos);

            vector<block_info> fetch_first_fork_from(vector<block_info> &bi);

            producer_schedule_type lib_active_producers() const;

            template<typename Signal, typename Arg>
            void emit(const Signal &s, Arg &&a);

            void set(pbft_state_ptr s);

            void set(pbft_checkpoint_state_ptr s);

            void prune(const pbft_state_ptr &h);

        };

    }
} /// namespace eosio::chain

FC_REFLECT(eosio::chain::block_info, (block_id)(block_num))
FC_REFLECT(eosio::chain::pbft_prepare,
           (uuid)(view)(block_num)(block_id)(public_key)(chain_id)(producer_signature)(timestamp))
FC_REFLECT(eosio::chain::pbft_commit,
           (uuid)(view)(block_num)(block_id)(public_key)(chain_id)(producer_signature)(timestamp))
FC_REFLECT(eosio::chain::pbft_view_change,
           (uuid)(current_view)(target_view)(prepared)(stable_checkpoint)(public_key)(chain_id)(producer_signature)(
                   timestamp))
FC_REFLECT(eosio::chain::pbft_new_view,
           (uuid)(view)(prepared)(stable_checkpoint)(view_changed)(public_key)(chain_id)(producer_signature)(timestamp))
FC_REFLECT(eosio::chain::pbft_state, (block_id)(block_num)(prepares)(should_prepared)(commits)(should_committed))
FC_REFLECT(eosio::chain::pbft_prepared_certificate, (block_id)(block_num)(prepares)(public_key)(producer_signature))
FC_REFLECT(eosio::chain::pbft_view_changed_certificate, (view)(view_changes)(public_key)(producer_signature))
FC_REFLECT(eosio::chain::pbft_checkpoint,
           (uuid)(block_num)(block_id)(public_key)(chain_id)(producer_signature)(timestamp))
FC_REFLECT(eosio::chain::pbft_stable_checkpoint, (block_num)(block_id)(checkpoints)(chain_id))
FC_REFLECT(eosio::chain::pbft_checkpoint_state, (block_id)(block_num)(checkpoints)(is_stable))