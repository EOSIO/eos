#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/table_object.hpp>
#include <cyberway/chaindb/journal.hpp>
#include <cyberway/chaindb/abi_info.hpp>

#include <eosio/chain/account_object.hpp>

/** Session exception is a critical errors and they doesn't handle by chain */
#define CYBERWAY_SESSION_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                            \
        if (BOOST_UNLIKELY( !(expr) )) {                                \
            elog(FORMAT, __VA_ARGS__);                                  \
            FC_THROW_EXCEPTION(session_exception, FORMAT, __VA_ARGS__); \
        }                                                               \
    FC_MULTILINE_MACRO_END

namespace cyberway { namespace chaindb {

    using fc::mutable_variant_object;

    enum class undo_stage {
        Unknown,
        New,
        Stack,
    }; // enum undo_stage

    class table_undo_stack;

    struct undo_state final {
        undo_state(table_undo_stack& table, revision_t rev): table_(table), revision_(rev) { }

        void set_next_pk(primary_key_t, primary_key_t);
        void move_next_pk(undo_state& src);
        void reset_next_pk();
        object_value next_pk_object(variant val = variant()) const;

        bool has_next_pk() const {
            return unset_primary_key != next_pk_;
        }

        primary_key_t next_pk() const {
            return next_pk_;
        }

        revision_t revision() const {
            return revision_;
        }

        void down_revision() {
            --revision_;
        }

        using pk_value_map_t_ = fc::flat_map<primary_key_t, object_value>;

        table_undo_stack& table_;
        pk_value_map_t_   new_values_;
        pk_value_map_t_   old_values_;
        pk_value_map_t_   removed_values_;

    private:
        primary_key_t     next_pk_      = unset_primary_key;
        primary_key_t     undo_next_pk_ = unset_primary_key;
        revision_t        revision_     = impossible_revision;
    }; // struct undo_state

    class table_undo_stack final: public table_object::object {
        undo_stage stage_    = undo_stage::Unknown; // <- state machine - changes via actions
        revision_t revision_ = impossible_revision; // <- part of state machine - changes via actions
        std::deque<undo_state> stack_;              // <- access depends on state machine

        fc::flat_map<revision_t, primary_key_t> undo_next_pk_map_; // <- map of undo_next_pk_ by revision

    public:
        table_undo_stack() = delete;

        table_undo_stack(const table_info& src, const revision_t rev)
        : object(src),
          stage_(undo_stage::New),
          revision_(rev) {
        }

        revision_t head_revision() const {
            if (stack_.empty()) {
                return 0;
            }
            return stack_.back().revision();
        }

        revision_t revision() const {
            return revision_;
        }

        void start_session(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(revision_ < rev,
                "Bad revision ${table_revision} (new ${revision}) for the table ${table}.",
                ("table", get_full_table_name())("table_revision", revision_)
                ("revision", rev));

            revision_ = rev;
            stage_ = undo_stage::New;
        }

        undo_state& head() {
            switch (stage_) {
                case undo_stage::New: {
                    stage_ = undo_stage::Stack;
                    stack_.emplace_back(*this, revision_);
                }

                case undo_stage::Stack:
                    return stack_.back();

                case undo_stage::Unknown:
                    break;
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a head.",
                ("table", get_full_table_name())("stage", stage_));
        }

        undo_state& tail() {
            if (!stack_.empty()) {
                return stack_.front();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a tail.",
                ("table", get_full_table_name())("stage", stage_));
        }

        undo_state& prev_state() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack:
                    CYBERWAY_SESSION_ASSERT(size() >= 2,
                        "The table ${table} doesn't have 2 states.", ("table", get_full_table_name()));
                    return stack_[stack_.size() - 2];

                case undo_stage::New:
                    CYBERWAY_SESSION_ASSERT(!empty(),
                        "The table ${table} doesn't have any state.", ("table", get_full_table_name()));
                    return stack_.back();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a previous state.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void squash() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack:
                    stack_.back().down_revision();

                case undo_stage::New: {
                    --revision_;
                    update_stage();
                    return;
                }
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on squashing of changes.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void undo() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack:
                    stack_.pop_back();

                case undo_stage::New: {
                    --revision_;
                    update_stage();
                    return;
                }
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on undoing of changes.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void commit() {
            if (!stack_.empty()) {
                stack_.pop_front();
                if (stack_.empty()) {
                    revision_ = impossible_revision;
                    stage_ = undo_stage::Unknown;
                }
            } else {
                CYBERWAY_SESSION_ASSERT(false,
                    "Wrong stage ${stage} of the table ${table} on committing of changes.",
                    ("table", get_full_table_name())("stage", stage_));
            }
        }

        primary_key_t set_undo_next_pk(const revision_t rev, const primary_key_t undo_pk) {
            return undo_next_pk_map_.emplace(rev, undo_pk).first->second;
        }

        void move_undo_next_pk(const revision_t dst, const revision_t src) {
            auto itr = undo_next_pk_map_.find(src);
            undo_next_pk_map_.emplace(dst, itr->second);
            undo_next_pk_map_.erase(src);
        }

        void remove_undo_next_pk(const revision_t rev) {
            for (auto itr = undo_next_pk_map_.begin(), etr = undo_next_pk_map_.end(); etr != itr;) {
                if (itr->first < rev) {
                    undo_next_pk_map_.erase(itr++);
                } else {
                    break;
                }
            }
        }

        size_t size() const {
            return stack_.size();
        }

        bool stack_empty() const {
            return stack_.empty();
        }

        bool empty() const {
            return stack_.empty();
        }

    private:
        void update_stage() {
            if (!empty() && revision_ == stack_.back().revision()) {
                stage_ = undo_stage::Stack;
            } else if (revision_ > 0) {
                stage_ = undo_stage::New;
            } else {
                revision_ = impossible_revision;
                stage_ = undo_stage::Unknown;
            }
        }

    }; // struct table_undo_stack

    void undo_state::set_next_pk(const primary_key_t next_pk, primary_key_t undo_pk) {
        next_pk_      = next_pk;
        undo_next_pk_ = table_.set_undo_next_pk(revision_, undo_pk);
    }

    void undo_state::move_next_pk(undo_state& src) {
        next_pk_      = src.next_pk_;
        undo_next_pk_ = src.undo_next_pk_;

        src.reset_next_pk();
        table_.move_undo_next_pk(revision_, src.revision_);
    }

    void undo_state::reset_next_pk() {
        next_pk_      = unset_primary_key;
        undo_next_pk_ = unset_primary_key;
    }

    object_value undo_state::next_pk_object(variant val) const {
        return object_value{{table_.info(), undo_next_pk_, undo_record::NextPk, revision_}, std::move(val)};
    }

    struct undo_stack::undo_stack_impl_ final {
        undo_stack_impl_(chaindb_controller& controller, driver_interface& driver, journal& jrnl, cache_map& cache)
        : controller_(controller),
          driver_(driver),
          journal_(jrnl),
          cache_(cache) {
        }

        void add_abi_tables(eosio::chain::abi_def& abi) {
            undo_abi_.structs.emplace_back( eosio::chain::struct_def{
               names::service_field, "",
               {{names::undo_pk_field,  "uint64"}}
            });

            undo_abi_.structs.emplace_back( eosio::chain::struct_def{
                names::undo_table, "",
                {{names::service_field, names::service_field}}
            });

            undo_abi_.tables.emplace_back( eosio::chain::table_def {
                names::undo_table,
                names::undo_table,
                {{"primary", true, {{ names::undo_pk_path , names::asc_order}} }},
            });

            abi.structs.insert(abi.structs.end(), undo_abi_.structs.begin(), undo_abi_.structs.end());
            abi.tables.emplace_back(undo_abi_.tables.front());

            auto& undo_def = undo_abi_.tables.front();

            auto& pk_order = undo_def.indexes.front().orders.front();
            pk_order.type = "uint64";
            pk_order.path = {names::service_field, names::undo_pk_field};

            undo_table_.table = &undo_def;
            undo_table_.pk_order = &pk_order;
        }

        void clear() {
            tables_.clear();
            revision_ = 0;
            tail_revision_ = 0;
        }

        revision_t revision() const {
            return revision_;
        }

        void set_revision(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(tables_.empty(), "Cannot set revision while there is an existing undo stack.");
            revision_ = rev;
            tail_revision_ = rev;
        }

        revision_t start_undo_session(bool enabled) {
            if (enabled) {
                ++revision_;
                for (auto& const_table: tables_) {
                    auto& table = const_cast<table_undo_stack&>(const_table); // not critical
                    table.start_session(revision_);
                }
                stage_ = undo_stage::Stack;
                return revision_;
            } else {
                return -1;
            }
        }

        bool enabled() const {
            switch (stage_) {
                case undo_stage::Stack:
                case undo_stage::New:
                    return true;

                case undo_stage::Unknown:
                    break;
            }
            return false;
        }

        void apply_changes(const revision_t apply_rev) {
            CYBERWAY_SESSION_ASSERT(apply_rev == revision_, "Wrong push revision ${apply_revision} != ${revision}",
                ("revision", revision_)("apply_revision", apply_rev));
            driver_.apply_all_changes();
        }

        void undo(const revision_t undo_rev) {
            for_tables([&](auto& table){
                if (!table.stack_empty()) {
                    undo(table, undo_rev, revision_);
                }
            });
            --revision_;
            if (revision_ == tail_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void squash(const revision_t squash_rev) {
            for_tables([&](auto& table){
                if (!table.stack_empty()) {
                    squash(table, squash_rev, revision_);
                }
            });
            --revision_;
            if (revision_ == tail_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void commit(const revision_t commit_rev) {
            for_tables([&](auto& table){
                commit(table, commit_rev);
            });
            tail_revision_ = commit_rev;
            if (revision_ == tail_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void undo_all() {
            for_tables([&](auto& table) {
                undo_all(table);
            });
            stage_ = undo_stage::Unknown;
            revision_ = tail_revision_;
        }

        void insert(const table_info& table, object_value obj) {
            obj.service.revision = revision_;
            if (enabled()) {
                insert(get_table(table), std::move(obj));
            } else {
                journal_.write_data(table, write_operation::insert(std::move(obj)));
            }
        }

        void update(const table_info& table, object_value orig_obj, object_value obj) {
            obj.service.revision = revision_;
            if (enabled()) {
                update(get_table(table), std::move(orig_obj), std::move(obj));
            } else {
                journal_.write_data(table, write_operation::update(std::move(obj)));
            }
        }

        void remove(const table_info& table, object_value orig_obj) {
            if (enabled()) {
                remove(get_table(table), std::move(orig_obj));
            } else {
                journal_.write_data(table, write_operation::remove(std::move(orig_obj)));
            }
        }

        void restore() try {
            if (start_revision <= revision_ || start_revision <= tail_revision_) {
                ilog( "Skip restore undo state, tail revision ${tail}, head revision = ${head}",
                    ("head", revision_)("tail", tail_revision_));
                return;
            }

            auto account_idx = controller_.get_index<eosio::chain::account_object, eosio::chain::by_name>();
            auto& abi_map = controller_.get_abi_map();

            auto get_system_table_def = [&](const auto& service) -> table_def {
                auto itr = abi_map.find(account_name());
                assert(itr != abi_map.end());

                auto def = itr->second.find_table(service.table);
                CYBERWAY_SESSION_ASSERT(nullptr != def, "The table ${table} doesn't exist on restore",
                    ("table", get_full_table_name(service)));
                return *def;
            };

            auto get_contract_table_def = [&](const auto& service) -> table_def {
                auto itr = account_idx.find(service.code);
                auto abi = itr->get_abi();
                auto dtr = std::find_if(abi.tables.begin(), abi.tables.end(), [&](auto& def){
                    return def.name == service.table;
                });
                CYBERWAY_SESSION_ASSERT(dtr != abi.tables.end(), "The table ${table} doesn't exist",
                    ("table", get_full_table_name(service)));
                return (*dtr);
            };

            auto get_state = [&](const auto& service) -> undo_state& {
                auto table = table_info(service.code, service.scope);
                auto def   = table_def();
                if (account_name() == service.code) {
                    def = get_system_table_def(service);
                } else {
                    def = get_contract_table_def(service);
                }
                table.table = &def;
                table.pk_order = &get_pk_order(table);

                auto& stack = get_table(table);
                if (stack.revision() != service.revision) {
                    stack.start_session(service.revision);
                }
                return stack.head();
            };

            auto index = index_info(undo_table_);
            index.index = &undo_table_.table->indexes.front();

            auto& cursor = driver_.lower_bound(std::move(index), {});
            for (; cursor.pk != end_primary_key; driver_.next(cursor)) {
                auto  obj   = driver_.object_at_cursor(cursor);
                auto  pk    = obj.pk();
                auto& state = get_state(obj.service);

                if (obj.service.undo_pk  >= undo_pk_ ) undo_pk_       = obj.service.undo_pk + 1;
                if (obj.service.revision >  revision_) revision_      = obj.service.revision;
                if (start_revision >= tail_revision_)  tail_revision_ = obj.service.revision - 1;

                switch (obj.service.undo_rec) {
                    case undo_record::NewValue:
                        state.new_values_.emplace(pk, std::move(obj));
                        break;

                    case undo_record::OldValue:
                        state.old_values_.emplace(pk, std::move(obj));
                        break;

                    case undo_record::RemovedValue:
                        state.removed_values_.emplace(pk, std::move(obj));
                        break;

                    case undo_record::NextPk: {
                        auto next_pk = obj.value.get_object()[names::next_pk_field].as_uint64();
                        auto undo_pk = obj.service.undo_pk;
                        state.set_next_pk(next_pk, undo_pk);
                        break;
                    }

                    case undo_record::Unknown:
                    default:
                        CYBERWAY_SESSION_ASSERT(false, "Unknown undo state on loading from DB");
                }
            }

            if (revision_ != tail_revision_) stage_ = undo_stage::Stack;
        } catch (const session_exception&) {
            throw;
        } catch (const std::exception& e) {
            CYBERWAY_SESSION_ASSERT(false, e.what());
        }

    private:
        void restore_undo_state(object_value& obj) {
            obj.service.revision = obj.service.undo_revision;
            obj.service.payer    = obj.service.undo_payer;
            obj.service.size     = obj.service.undo_size;
        }

        void undo(table_undo_stack& table, const revision_t undo_rev, const revision_t test_rev) {
            if (undo_rev > table.head_revision()) {
                table.undo();
                return;
            }

            auto& head = table.head();

            CYBERWAY_SESSION_ASSERT(head.revision() == undo_rev && undo_rev == test_rev,
                "Wrong undo revision ${undo_revision} != (${revision}, ${test_revision}) "
                "for the table ${table} for the scope '${scope}",
                ("revision", head.revision())("undo_revision", undo_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table.info()))("scope", get_scope_name(table.info())));

            auto ctx = journal_.create_ctx(table.info());

            for (auto& obj: head.old_values_) {
                auto undo_pk = obj.second.clone_service();

                restore_undo_state(obj.second);
                cache_.emplace(table.info(), obj.second);

                journal_.write(ctx,
                    write_operation::update(undo_rev, std::move(obj.second)),
                    write_operation::remove(undo_rev, std::move(undo_pk)));
            }

            for (auto& obj: head.new_values_) {
                cache_.remove(table.info(), obj.first);
                journal_.write(ctx,
                    write_operation::remove(undo_rev, obj.second.clone_service()),
                    write_operation::remove(undo_rev, obj.second.clone_service()));
            }

            for (auto& obj: head.removed_values_) {
                auto undo_pk = obj.second.clone_service();

                restore_undo_state(obj.second);
                cache_.emplace(table.info(), obj.second);

                journal_.write(ctx,
                    write_operation::insert(std::move(obj.second)),
                    write_operation::remove(undo_rev, std::move(undo_pk)));
            }

            if (head.has_next_pk()) {
                cache_.set_next_pk(table.info(), head.next_pk());
            }

            remove_next_pk(ctx, table, head);

            table.undo();
        }

        template <typename Write>
        void process_state(undo_state& state, Write&& write) const {
            const auto rev = state.revision();
            for (auto& obj: state.old_values_)     write(true,  obj.second, rev);
            for (auto& obj: state.new_values_)     write(true,  obj.second, rev);
            for (auto& obj: state.removed_values_) write(false, obj.second, rev);
        }

        template <typename Ctx>
        void remove_next_pk(Ctx& ctx, const table_undo_stack& table, undo_state& state) const {
            if (!state.has_next_pk()) return;

            journal_.write_undo(ctx, write_operation::remove(state.revision(), state.next_pk_object()));
            state.reset_next_pk();
        }

        void squash_state(table_undo_stack& table, undo_state& state) const {
            auto ctx = journal_.create_ctx(table.info());

            process_state(state, [&](bool has_data, auto& obj, auto& rev) {
                if (has_data) journal_.write_data(ctx, write_operation::revision(rev, obj.clone_service()));
                journal_.write_undo(ctx, write_operation::revision(rev, obj.clone_service()));
                obj.service.revision = rev - 1;
            });

            if (state.has_next_pk()) {
                journal_.write_undo(ctx, write_operation::revision(state.revision(), state.next_pk_object()));
                table.move_undo_next_pk(state.revision() - 1, state.revision());
            }

            table.squash();
        }

        void remove_state(table_undo_stack& table, undo_state& state) const {
            auto ctx = journal_.create_ctx(table.info());

            process_state(state, [&](bool has_data, auto& obj, auto& rev) {
                if (has_data) journal_.write_data(ctx, write_operation::revision(rev, obj.clone_service()));
                journal_.write_undo(ctx, write_operation::remove(rev, obj.clone_service()));
            });

            remove_next_pk(ctx, table, state);

            table.undo();
        }

        void squash(table_undo_stack& table, const revision_t squash_rev, const revision_t test_rev) {
            if (squash_rev > table.head_revision()) {
                table.squash();
                return;
            }

            auto& state = table.head();
            CYBERWAY_SESSION_ASSERT(state.revision() == squash_rev && squash_rev == test_rev,
                "Wrong squash revision ${squash_revision} != (${revision}, ${test_revision})"
                "for the table ${table} for the scope '${scope}",
                ("revision", state.revision())("squash_revision", squash_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table.info()))("scope", get_scope_name(table.info())));

            // Only one stack item
            if (table.size() == 1) {
                if (state.revision() > tail_revision_) {
                    squash_state(table, state);
                } else {
                    remove_state(table, state);
                }
                return;
            }

            auto& prev_state = table.prev_state();

            // Two stack items but they are not neighbours
            if (prev_state.revision() != state.revision() - 1) {
                squash_state(table, state);
                return;
            }

            // An object's relationship to a state can be:
            // in new_ids            : new
            // in old_values (was=X) : upd(was=X)
            // in removed (was=X)    : del(was=X)
            // not in any of above   : nop
            //
            // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
            //
            //                   |--------------------- B ----------------------|
            //
            //                +------------+------------+------------+------------+
            //                | new        | upd(was=Y) | del(was=Y) | nop        |
            //   +------------+------------+------------+------------+------------+
            // / | new        | N/A        | new       A| nop       C| new       A|
            // | +------------+------------+------------+------------+------------+
            // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
            // A +------------+------------+------------+------------+------------+
            // | | del(was=X) | upd(was=X) | N/A        | N/A        | del(was=X)A|
            // | +------------+------------+------------+------------+------------+
            // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
            //   +------------+------------+------------+------------+------------+
            //
            // Each entry was composed by labelling what should occur in the given case.
            //
            // Type A means the composition of states contains the same entry as the first of the two merged states for that object.
            // Type B means the composition of states contains the same entry as the second of the two merged states for that object.
            // Type C means the composition of states contains an entry different from either of the merged states for that object.
            // Type N/A means the composition of states violates causal timing.
            // Type AB means both type A and type B simultaneously.
            //
            // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the composition of
            // state A and B.
            //
            // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged state.
            // Type B (and AB) can be implemented by copying from state to prev_state.
            // Type C needs special case-by-case logic.
            // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
            // (a serious logic error which should never happen).
            //

            // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three containers.

            auto ctx = journal_.create_ctx(table.info());

            for (auto& obj: state.old_values_) {
                const auto pk = obj.second.pk();
                bool  exists = false;

                // 1. new+upd -> new, type A
                auto nitr = prev_state.new_values_.find(pk);
                if (prev_state.new_values_.end() != nitr) {
                    exists = true;
                    copy_undo_object(nitr->second, obj.second);
                } else {
                    // 2. upd(was=X) + upd(was=Y) -> upd(was=X), type A
                    auto oitr = prev_state.old_values_.find(pk);
                    if (prev_state.old_values_.end() != oitr) {
                        exists = true;
                        copy_undo_object(oitr->second, obj.second);
                    }
                }

                if (exists) {
                    journal_.write(ctx,
                        write_operation::revision(state.revision(), obj.second.clone_service()),
                        write_operation::remove(  state.revision(), obj.second.clone_service()));
                    continue;
                }

                // del+upd -> N/A
                CYBERWAY_SESSION_ASSERT(!prev_state.removed_values_.count(pk),
                    "UB for the table ${table}: Delete + Update", ("table", get_full_table_name(table.info())));

                // nop+upd(was=Y) -> upd(was=Y), type B

                journal_.write(ctx,
                    write_operation::revision(state.revision(), obj.second.clone_service()),
                    write_operation::revision(state.revision(), obj.second.clone_service()));

                obj.second.service.revision = prev_state.revision();
                
                prev_state.old_values_.emplace(pk, std::move(obj.second));
            }

            for (auto& obj: state.new_values_) {
                const auto pk = obj.second.pk();

                auto ritr = prev_state.removed_values_.find(pk);
                if (ritr != prev_state.removed_values_.end()) {
                    // del(was=X) + ins(was=Y) -> up(was=X)

                    journal_.write_undo(ctx, write_operation::remove(state.revision(), obj.second.clone_service()));

                    ritr->second.service.undo_rec = undo_record::OldValue;
                    journal_.write_undo(ctx, write_operation::update(ritr->second));

                    prev_state.old_values_.emplace(std::move(*ritr));
                    prev_state.removed_values_.erase(ritr);
                } else {
                    // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new

                    journal_.write(ctx,
                        write_operation::revision(state.revision(), obj.second.clone_service()),
                        write_operation::revision(state.revision(), obj.second.clone_service()));

                    obj.second.service.revision = prev_state.revision();

                    prev_state.new_values_.emplace(pk, std::move(obj.second));
                }
            }

            // *+del
            for (auto& obj: state.removed_values_) {
                const auto pk = obj.second.pk();

                // new + del -> nop (type C)
                auto nitr = prev_state.new_values_.find(pk);
                if (nitr != prev_state.new_values_.end()) {
                    prev_state.new_values_.erase(nitr);

                    journal_.write_undo(ctx, write_operation::remove(state.revision(), obj.second.clone_service()));
                    continue;
                }

                // upd(was=X) + del(was=Y) -> del(was=X)
                auto oitr = prev_state.old_values_.find(pk);
                if (oitr != prev_state.old_values_.end()) {
                    prev_state.removed_values_.emplace(std::move(*oitr));
                    prev_state.old_values_.erase(oitr);

                    journal_.write_undo(ctx, write_operation::remove(state.revision(), obj.second.clone_service()));
                    continue;
                }

                // del + del -> N/A
                CYBERWAY_SESSION_ASSERT(!prev_state.removed_values_.count(pk),
                    "UB for the table ${table}: Delete + Delete", ("table", get_full_table_name(table.info())));

                // nop + del(was=Y) -> del(was=Y)

                journal_.write_undo(ctx, write_operation::revision(state.revision(), obj.second.clone_service()));
                
                obj.second.service.revision = prev_state.revision();
                
                prev_state.removed_values_.emplace(std::move(obj));
            }

            if (state.has_next_pk()) {
                if (!prev_state.has_next_pk()) {
                    journal_.write_undo(ctx, write_operation::revision(state.revision(), state.next_pk_object()));
                    prev_state.move_next_pk(state);
                } else {
                    remove_next_pk(ctx, table, state);
                }
            }

            table.undo();
        }

        void commit(table_undo_stack& table, const revision_t commit_rev) {
            table.remove_undo_next_pk(commit_rev);
            if (table.empty()) return;

            auto ctx = journal_.create_ctx(table.info());

            while (!table.empty()) {
                auto& state = table.tail();

                if (state.revision() > commit_rev) return;

                process_state(state, [&](bool, auto& obj, auto& rev){
                    journal_.write_undo(ctx, write_operation::remove(rev, obj.clone_service()));
                });
                remove_next_pk(ctx, table, state);

                table.commit();
            }
        }

        void undo_all(table_undo_stack& table) {
            while (table.size() > 1) {
                squash(table, table.revision(), table.revision());
            }
            if (!table.stack_empty()) {
                undo(table, table.revision(), table.revision());
            }
        }

        void copy_undo_object(object_value& dst, const object_value& src) {
            dst.service.payer = src.service.payer;
            dst.service.size  = src.service.size;
        }

        void copy_undo_object(object_value& dst, const object_value& src, const undo_record rec) {
            copy_undo_object(dst, src);
            dst.service.undo_rec = rec;
        }

        void init_undo_object(object_value& dst, undo_record rec) {
            dst.service.undo_revision = dst.service.revision;
            dst.service.undo_payer    = dst.service.payer;
            dst.service.undo_size     = dst.service.size;

            dst.service.revision      = revision_;
            dst.service.undo_pk       = generate_undo_pk();
            dst.service.undo_rec      = rec;
        }

        void insert(table_undo_stack& table, object_value obj) {
            const auto pk = obj.pk();
            auto& head = table.head();
            auto  ctx = journal_.create_ctx(table.info());

            journal_.write_data(ctx, write_operation::insert(obj));

            auto ritr = head.removed_values_.find(pk);
            if (head.removed_values_.end() != ritr) {
                copy_undo_object(ritr->second, obj, undo_record::OldValue);
                journal_.write_undo(ctx, write_operation::update(ritr->second.clone_service()));

                head.old_values_.emplace(std::move(*ritr));
                head.removed_values_.erase(ritr);
                return;
            }

            init_undo_object(obj, undo_record::NewValue);
            journal_.write_undo(ctx, write_operation::insert(obj.clone_service()));
            head.new_values_.emplace(pk, std::move(obj));

            if (!head.has_next_pk()) {
                head.set_next_pk(pk, generate_undo_pk());

                auto val = mutable_variant_object{names::next_pk_field, pk};
                journal_.write_undo(ctx, write_operation::insert(head.next_pk_object(std::move(val))));
            }
        }

        void update(table_undo_stack& table, object_value orig_obj, object_value obj) {
            const auto pk = orig_obj.pk();
            auto& head = table.head();
            auto  ctx = journal_.create_ctx(table.info());

            journal_.write_data(ctx, write_operation::update(obj));

            auto nitr = head.new_values_.find(pk);
            if (head.new_values_.end() != nitr) {
                copy_undo_object(nitr->second, obj);
                journal_.write_undo(ctx, write_operation::update(nitr->second.clone_service()));
                return;
            }

            auto oitr = head.old_values_.find(pk);
            if (head.old_values_.end() != oitr) {
                copy_undo_object(oitr->second, obj);
                journal_.write_data(ctx, write_operation::update(std::move(obj)));
                return;
            }

            init_undo_object(orig_obj, undo_record::OldValue);
            copy_undo_object(orig_obj, obj);
            journal_.write_undo(ctx, write_operation::insert(orig_obj));
            head.old_values_.emplace(pk, std::move(orig_obj));
        }

        void remove(table_undo_stack& table, object_value orig_obj) {
            const auto pk = orig_obj.pk();
            auto& head = table.head();
            auto  ctx = journal_.create_ctx(table.info());

            journal_.write_data(ctx, write_operation::remove(orig_obj.clone_service()));

            auto nitr = head.new_values_.find(pk);
            if (head.new_values_.end() != nitr) {
                journal_.write_undo(ctx, write_operation::remove(std::move(nitr->second)));
                head.new_values_.erase(nitr);
                return;
            }

            auto oitr = head.old_values_.find(pk);
            if (oitr != head.old_values_.end()) {
                oitr->second.service.undo_rec = undo_record::RemovedValue;
                journal_.write_undo(ctx, write_operation::update(oitr->second));

                head.removed_values_.emplace(std::move(*oitr));
                head.old_values_.erase(oitr);
                return;
            }

            init_undo_object(orig_obj, undo_record::RemovedValue);
            journal_.write_undo(ctx, write_operation::insert(orig_obj));
            head.removed_values_.emplace(pk, std::move(orig_obj));
        }

        table_undo_stack& get_table(const table_info& table) {
            auto itr = table_object::find(tables_, table);
            if (tables_.end() != itr) {
                return const_cast<table_undo_stack&>(*itr); // not critical
            }

            return table_object::emplace(tables_, table, revision_);
        }

        template <typename Lambda>
        void for_tables(Lambda&& lambda) try {
            for (auto itr = tables_.begin(), etr = tables_.end(); etr != itr; ) {
                auto& table = const_cast<table_undo_stack&>(*itr);
                lambda(table);

                if (table.empty()) {
                    tables_.erase(itr++);
                } else {
                    ++itr;
                }
            }
        } FC_LOG_AND_RETHROW()

        primary_key_t generate_undo_pk() {
            if (undo_pk_ > 1'000'000'000) {
                undo_pk_ = 1;
            }
            return undo_pk_++;
        }

        using index_t_ = table_object::index<table_undo_stack>;

        undo_stage          stage_ = undo_stage::Unknown;
        revision_t          revision_ = 0;
        revision_t          tail_revision_ = 0;
        abi_def             undo_abi_;
        table_info          undo_table_ = {account_name(), account_name()};
        primary_key_t       undo_pk_ = 1;
        chaindb_controller& controller_;
        driver_interface&   driver_;
        journal&            journal_;
        cache_map&          cache_;
        index_t_            tables_;
    }; // struct undo_stack::undo_stack_impl_

    undo_stack::undo_stack(chaindb_controller& controller, driver_interface& driver, journal& jrnl, cache_map& cache)
    : impl_(new undo_stack_impl_(controller, driver, jrnl, cache)) {
    }

    undo_stack::~undo_stack() = default;

    void undo_stack::add_abi_tables(eosio::chain::abi_def& abi) const {
        impl_->add_abi_tables(abi);
    }

    void undo_stack::restore() {
        impl_->restore();
    }

    void undo_stack::clear() {
        impl_->clear();
    }

    chaindb_session undo_stack::start_undo_session(bool enabled) {
        return chaindb_session(*this, impl_->start_undo_session(enabled));
    }

    void undo_stack::set_revision(const revision_t rev) {
        impl_->set_revision(rev);
    }

    revision_t undo_stack::revision() const {
        return impl_->revision();
    }

    bool undo_stack::enabled() const {
        return impl_->enabled();
    }

    void undo_stack::apply_changes(const revision_t rev) {
        impl_->apply_changes(rev);
    }

    void undo_stack::undo(const revision_t undo_rev) {
        impl_->undo(undo_rev);
    }

    void undo_stack::squash(const revision_t squash_rev) {
        impl_->squash(squash_rev);
    }

    void undo_stack::commit(const revision_t commit_rev) {
        impl_->commit(commit_rev);
    }

    void undo_stack::undo_all() {
        impl_->undo_all();
    }

    void undo_stack::undo() {
        impl_->undo(impl_->revision());
    }

    void undo_stack::insert(const table_info& table, object_value obj) {
        impl_->insert(table, std::move(obj));
    }

    void undo_stack::update(const table_info& table, object_value orig_obj, object_value obj) {
        impl_->update(table,  std::move(orig_obj), std::move(obj));
    }

    void undo_stack::remove(const table_info& table, object_value orig_obj) {
        impl_->remove(table,  std::move(orig_obj));
    }

    //------

    chaindb_session::chaindb_session(undo_stack& stack, revision_t rev)
    : stack_(stack),
      apply_(true),
      revision_(rev) {
        if (rev == -1) {
            apply_ = false;
        }
    }

    chaindb_session::chaindb_session(chaindb_session&& mv)
    : stack_(mv.stack_),
      apply_(mv.apply_),
      revision_(mv.revision_) {
        mv.apply_ = false;
    }

    chaindb_session::~chaindb_session() {
        if (apply_) {
            stack_.undo(revision_);
        }
    }

    void chaindb_session::apply_changes() {
        if (apply_) {
            stack_.apply_changes(revision_);
        }
        apply_ = false;
    }

    void chaindb_session::push() {
        apply_ = false;
    }

    void chaindb_session::squash() {
        if (apply_) {
            stack_.squash(revision_);
        }
        apply_ = false;
    }

    void chaindb_session::undo() {
        if (apply_) {
            stack_.undo(revision_);
        }
        apply_ = false;
    }

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM(cyberway::chaindb::undo_stage, (Unknown)(New)(Stack))
