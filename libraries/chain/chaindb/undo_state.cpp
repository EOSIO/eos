#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/table_object.hpp>
#include <cyberway/chaindb/journal.hpp>

/** Session exception is a critical errors and they doesn't handle by chain */
#define CYBERWAY_SESSION_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                            \
        if (!(expr)) {                                                  \
            elog(FORMAT, __VA_ARGS__);                                  \
            FC_THROW_EXCEPTION(session_exception, FORMAT, __VA_ARGS__); \
        }                                                               \
    FC_MULTILINE_MACRO_END

namespace cyberway { namespace chaindb {

    using fc::mutable_variant_object;

    namespace bmi = boost::multi_index;

    enum class undo_stage {
        Unknown,
        New,
        Stack,
        Rollback,
    };

    enum class undo_record {
        OldValue,
        RemovedValue,
        NewValue,
        NextPk,
    };

    struct undo_state final {
        undo_state() = default;
        undo_state(revision_t rev): revision(rev) { }

        using pk_set_t = fc::flat_set<primary_key_t>;
        using pk_value_map_t = fc::flat_map<primary_key_t, variant>;

        revision_t     revision = impossible_revision;
        primary_key_t  next_pk = unset_primary_key;
        pk_value_map_t old_values;
        pk_value_map_t removed_values;
        pk_set_t       new_ids;
    }; // struct undo_state

    class table_undo_stack final: public table_object::object {
        undo_stage stage_    = undo_stage::Unknown; // <- state machine - changes via actions
        revision_t revision_ = impossible_revision; // <- part of state machine - changes via actions

        std::deque<undo_state> stack_;              // <- access depends on state machine

    public:
        table_undo_stack() = delete;

        table_undo_stack(const table_info& src, const revision_t rev)
        : object(src),
          stage_(undo_stage::New),
          revision_(rev) {
        }

        revision_t revision() const {
            return revision_;
        }

        void start_session(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(!empty(),
                "The stack of the table ${table} is empty.", ("table", get_full_table_name()));

            CYBERWAY_SESSION_ASSERT(stack_.back().revision < rev,
                "Bad revision ${table_revision} (new ${revision}) for the table ${table}.",
                ("table", get_full_table_name())("table_revision", stack_.back().revision)
                ("revision", rev));

            revision_ = rev;
            stage_ = undo_stage::New;
        }

        undo_state& head() {
            switch (stage_) {
                case undo_stage::New: {
                    stage_ = undo_stage::Stack;
                    stack_.emplace_back(revision_);
                }

                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    if (stack_.empty()) {
                        break;
                    }

                case undo_stage::Stack:
                    return stack_.back();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a head.",
                ("table", get_full_table_name())("stage", stage_));
        }

        undo_state& prev_state() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
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

        undo_state& tail() {
            return stack_.front();
        }

        void commit() {
           stack_.pop_front();
        }

        void undo() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    if (stack_.empty()) {
                        break;
                    }

                case undo_stage::Stack: {
                    stack_.pop_back();
                    if (empty()) {
                        stage_    = undo_stage::Rollback;
                        revision_ = impossible_revision; // for validation in other places
                        return; // table should be removed
                    }
                }

                case undo_stage::New:
                    stage_    = undo_stage::Rollback;
                    revision_ = stack_.back().revision;
                    return;
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on undoing of changes.",
                ("table", get_full_table_name())("stage", stage_));
        }

        size_t size() const {
            return stack_.size();
        }

        bool empty() const {
            return stack_.empty();
        }

    }; // struct table_undo_stack

    struct undo_stack::undo_stack_impl_ final {
        undo_stack_impl_(driver_interface& driver, journal& jrl, cache_map& cache)
        : driver_(driver),
          journal_(jrl),
          cache_(cache) {
        }

        void clear() {
            tables_.clear();
            revision_ = 0;
        }

        void set_revision(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(tables_.empty(), "Cannot set revision while there is an existing undo stack.");
            revision_ = rev;
        }

        revision_t start_undo_session(bool enabled) {
            if (enabled) {
                ++revision_;
                for (auto& const_table: tables_) {
                    auto& table = const_cast<table_undo_stack&>(const_table); // not critical
                    table.start_session(revision_);
                }
                stage_ = undo_stage::New;
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
                case undo_stage::Rollback:
                    break;
            }
            return false;
        }

        void apply_changes(const revision_t apply_rev) {
            CYBERWAY_SESSION_ASSERT(apply_rev == revision_, "Wrong push revision ${apply_revision} != ${revision}",
                ("revision", revision_)("apply_revision", apply_rev));
            driver_.apply_all_changes();
        }

        void push(const revision_t push_rev) {
            CYBERWAY_SESSION_ASSERT(push_rev == revision_, "Wrong push revision ${push_revision} != ${revision}",
                ("revision", revision_)("push_revision", push_rev));
            driver_.apply_all_changes();
            stage_ = undo_stage::Unknown;
        }

        void undo(const revision_t undo_rev) {
            for_tables([&](auto& table){
                undo(table, undo_rev, revision_);
            });
            --revision_;
            stage_ = undo_stage::Rollback;
        }

        void squash(const revision_t squash_rev) {
            for_tables([&](auto& table){
                squash(table, squash_rev, revision_);
            });
            --revision_;
            stage_ = undo_stage::Unknown;
        }

        void commit(const revision_t commit_rev) {
            for_tables([&](auto& table){
                commit(table, commit_rev);
            });
            if (tables_.empty()) {
                stage_ = undo_stage::Unknown;
            }
        }

        void undo_all() {
            for_tables([&](auto& table) {
                undo_all(table);
            });
            stage_ = undo_stage::Rollback;
        }

        void insert(const table_info& table, const primary_key_t pk, variant value) {
            if (enabled()) {
                insert(get_table(table), pk, std::move(value));
            } else {
                journal_.write(table, pk, {write_operation::Insert, revision_ /*set_rev*/, std::move(value)}, {});
            }
        }

        void update(const table_info& table, const primary_key_t pk, variant orig_value, variant value) {
            if (enabled()) {
                update(get_table(table), pk, std::move(orig_value), std::move(value));
            } else {
                journal_.write(table, pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
            }
        }

        void remove(const table_info& table, const primary_key_t pk, variant orig_value) {
            if (enabled()) {
                remove(get_table(table), pk, std::move(orig_value));
            } else {
                journal_.write(table, pk, {write_operation::Delete}, {});
            }
        }

        void undo(table_undo_stack& table, const revision_t undo_rev, const revision_t test_rev) {
            if (undo_rev > table.revision()) return;

            const auto& head = table.head();

            CYBERWAY_SESSION_ASSERT(head.revision == undo_rev && undo_rev == test_rev,
                "Wrong undo revision ${undo_revision} != (${revision}, ${test_revision}) "
                "for the table ${table} for the scope '${scope}",
                ("revision", head.revision)("undo_revision", undo_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table))("scope", get_scope_name(table)));

            for (auto& item: head.old_values) {
                cache_.update(table.info(), item.first, item.second);
                journal_.write(table.info(), item.first,
                    {write_operation::Update, undo_rev - 1 /*set_rev*/, undo_rev /*find_rev*/, std::move(item.second)},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            for (auto pk: head.new_ids) {
                cache_.remove(table.info(), pk);
                journal_.write(table.info(), pk,
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            for (auto& item: head.removed_values) {
                cache_.remove(table.info(), item.first);
                // insert
                journal_.write(table.info(), item.first,
                    {write_operation::Insert, undo_rev - 1 /*set_rev*/, std::move(item.second)},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            if (unset_primary_key != head.next_pk) {
                cache_.set_next_pk(table.info(), head.next_pk);
            }

            table.undo();
        }

        void squash_state(table_undo_stack& table, const undo_state& state) {
            auto update = [&](auto& pk) {
                journal_.write(table.info(), pk,
                    {write_operation::UpdateRevision, state.revision - 1 /*set_rev*/, state.revision /*find_rev*/},
                    {write_operation::Delete,        impossible_revision /*set_rev*/, state.revision /*find_rev*/});
            };

            for (const auto& obj: state.old_values)     update(obj.first);
            for (const auto& pk:  state.new_ids)        update(pk);
            for (const auto& obj: state.removed_values) update(obj.first);

            table.undo();
        }

        void squash(table_undo_stack& table, const revision_t squash_rev, const revision_t test_rev) {
            if (squash_rev > table.revision()) return;

            auto& state = table.head();
            CYBERWAY_SESSION_ASSERT(state.revision == squash_rev && squash_rev == test_rev,
                "Wrong squash revision ${squash_revision} != (${revision}, ${test_revision})"
                "for the table ${table} for the scope '${scope}",
                ("revision", state.revision)("squash_revision", squash_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table))("scope", get_scope_name(table)));

            // Only one stack item
            if (table.size() == 1) {
                squash_state(table, state);
                return;
            }

            auto& prev_state = table.prev_state();

            // Two stack items but they are not neighbours
            if (prev_state.revision != state.revision + 1) {
                --state.revision;
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
            // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
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

            for (const auto& obj: state.old_values) {
                // new+upd -> new, type A
                if (prev_state.new_ids.find(obj.first) != prev_state.new_ids.end()) {
                    journal_.write(table.info(), obj.first,
                        {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/},
                        {write_operation::Delete,         impossible_revision /*set_rev*/, state.revision /*find_rev*/});
                    continue;
                }

                // upd(was=X) + upd(was=Y) -> upd(was=X), type A
                if (prev_state.old_values.find(obj.first) != prev_state.old_values.end()) {
                    journal_.write(table.info(), obj.first,
                        {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/},
                        {write_operation::Delete,         impossible_revision /*set_rev*/, state.revision /*find_rev*/});
                    continue;
                }

                // del+upd -> N/A
                CYBERWAY_SESSION_ASSERT(prev_state.removed_values.find(obj.first) == prev_state.removed_values.end(),
                    "UB for the table ${table}: Delete + Update", ("table", get_full_table_name(table.info())));

                // nop+upd(was=Y) -> upd(was=Y), type B
                prev_state.old_values.emplace(obj.first, std::move(obj.second));

                journal_.write(table.info(), obj.first,
                    {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/},
                    {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/});
            }

            // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
            for (const auto& id: state.new_ids) {
                prev_state.new_ids.insert(id);

                journal_.write(table.info(), id,
                    {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/},
                    {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/});
            }

            if (unset_primary_key != state.next_pk) {
                prev_state.next_pk = state.next_pk;
            }

            // *+del
            for (const auto& obj: state.removed_values) {
                // new + del -> nop (type C)
                if (prev_state.new_ids.find(obj.first) != prev_state.new_ids.end()) {
                    prev_state.new_ids.erase(obj.first);

                    journal_.write(table.info(), obj.first, {},
                        {write_operation::Delete, impossible_revision /*set_rev*/, prev_state.revision /*find_rev*/});
                    continue;
                }

                // upd(was=X) + del(was=Y) -> del(was=X)
                auto it = prev_state.old_values.find(obj.first);
                if (it != prev_state.old_values.end()) {
                    prev_state.removed_values.emplace(std::move(*it));
                    prev_state.old_values.erase(obj.first);

                    journal_.write(table.info(), obj.first, {},
                        {write_operation::Delete, impossible_revision /*set_rev*/, prev_state.revision /*find_rev*/});
                    continue;
                }

                // del + del -> N/A
                CYBERWAY_SESSION_ASSERT(prev_state.removed_values.find(obj.first) == prev_state.removed_values.end(),
                    "UB for the table ${table}: Delete + Delete", ("table", get_full_table_name(table)));

                // nop + del(was=Y) -> del(was=Y)
                prev_state.removed_values.emplace(std::move(obj)); //[obj.second->id] = std::move(obj.second);

                journal_.write(table.info(), obj.first, {},
                    {write_operation::UpdateRevision, prev_state.revision /*set_rev*/, state.revision /*find_rev*/});
            }

            table.undo();
        }

        void commit(table_undo_stack& table, const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(!table.empty(),
                "Stack of the table ${table} is empty.", ("table", table.get_full_table_name()));

            auto update = [&](auto& pk, auto& rev) {
                journal_.write(table.info(), pk, {},
                   {write_operation::Delete, impossible_revision /*set_rev*/, rev /*find_rev*/});
            };

            while (!table.empty()) {
                auto& state = table.tail();

                if (state.revision > rev) return;

                for (const auto& obj: state.old_values)     update(obj.first, state.revision);
                for (const auto& pk:  state.new_ids)        update(pk, state.revision);
                for (const auto& obj: state.removed_values) update(obj.first, state.revision);

                table.commit();
            }
        }

        void undo_all(table_undo_stack& table) {
            while (table.size() > 1) {
                squash(table, table.revision(), table.revision());
            }
            undo(table, table.revision(), table.revision());
        }

        void add_undo_fields(mutable_variant_object& obj, const table_info& table, const undo_record type) const {
            obj(get_record_field_name(), type);
            obj(get_code_field_name(), get_code_name(table));
            obj(get_table_field_name(), get_table_name(table));
        }

        variant create_undo_value(const table_info& table, const undo_record type, const variant& value) const {
            mutable_variant_object obj(value.get_object());
            add_undo_fields(obj, table, type);
            return variant(std::move(obj));
        }

        void insert(table_undo_stack& table, const primary_key_t pk, variant value) {
            auto& head = table.head();
            head.new_ids.insert(pk);

            {
                mutable_variant_object obj(value.get_object());
                add_undo_fields(obj, table.info(), undo_record::NewValue);
                journal_.write(table.info(), pk, {write_operation::Insert, revision_ /*set_rev*/, std::move(value)},
                    {write_operation::Insert, revision_ /*set_rev*/, std::move(obj)});
            }

            if (unset_primary_key == head.next_pk) {
                head.next_pk = pk - 1;

                mutable_variant_object obj;
                add_undo_fields(obj, table.info(), undo_record::NextPk);
                obj(get_pk_field_name(), pk);

                journal_.write(table.info(), pk, {}, {write_operation::Insert, revision_ /*set_rev*/, std::move(obj)});
            }
        }

        void update(table_undo_stack& table, const primary_key_t pk, variant orig_value, variant value) {
            auto& head = table.head();

            if (head.new_ids.find(pk) != head.new_ids.end()) {
                journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
                return;
            }

            auto itr = head.old_values.find(pk);
            if (itr != head.old_values.end()) {
                journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
                return;
            }

            auto undo_value = create_undo_value(table.info(), undo_record::OldValue, orig_value);

            journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)},
                {write_operation::Insert, revision_ /*set_rev*/, std::move(undo_value)});

            head.old_values.emplace(pk, std::move(value));
        }

        void remove(table_undo_stack& table, const primary_key_t pk, variant orig_value) {
            auto& head = table.head();

            if (head.new_ids.count(pk)) {
                head.new_ids.erase(pk);

                journal_.write(table.info(), pk, {write_operation::Delete, revision_ /*set_rev*/},
                    {write_operation::Delete, revision_ /*set_rev*/});
                return;
            }

            auto undo_value = create_undo_value(table.info(), undo_record::RemovedValue, orig_value);

            journal_.write(table.info(), pk, {write_operation::Delete, revision_ /*set_rev*/},
                {write_operation::Insert, revision_ /*set_rev*/, std::move(undo_value)});

            auto itr = head.old_values.find(pk);
            if (itr != head.old_values.end()) {
                head.removed_values.emplace(std::move(*itr));
                head.old_values.erase(pk);
                return;
            }

            head.removed_values.emplace(pk, std::move(orig_value));
        }

        table_undo_stack& get_table(const table_info& table) {
            auto itr = table_object::find(tables_, table);
            if (tables_.end() != itr) return const_cast<table_undo_stack&>(*itr); // not critical

            return table_object::emplace(tables_, table, revision_);
        }

        template <typename Lambda>
        void for_tables(Lambda&& lambda) {
            for (auto itr = tables_.begin(), etr = tables_.end(); etr != itr; ) {
                auto& table = const_cast<table_undo_stack&>(*itr);
                lambda(table);

                if (table.empty()) {
                    tables_.erase(itr++);
                } else {
                    ++itr;
                }
            }
        }

        using index_t_ = table_object::index<table_undo_stack>;

        undo_stage stage_ = undo_stage::Unknown;
        revision_t revision_ = 0;
        driver_interface& driver_;
        journal& journal_;
        cache_map& cache_;
        index_t_ tables_;
    }; // struct undo_stack::undo_stack_impl_

    undo_stack::undo_stack(driver_interface& driver, journal& j, cache_map& cache)
    : impl_(new undo_stack_impl_(driver, j, cache)) {
    }

    undo_stack::~undo_stack() = default;

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
        return impl_->revision_;
    }

    bool undo_stack::enabled() const {
        return impl_->enabled();
    }

    void undo_stack::apply_changes(const revision_t rev) {
        impl_->apply_changes(rev);
    }

    void undo_stack::push(const revision_t push_rev) {
        impl_->push(push_rev);
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
        impl_->undo(impl_->revision_);
    }

    void undo_stack::insert(const table_info& table, const primary_key_t pk, variant value) {
        impl_->insert(table, pk, std::move(value));
    }

    void undo_stack::update(const table_info& table, const primary_key_t pk, variant orig_value, variant value) {
        impl_->update(table, pk, std::move(orig_value), std::move(value));
    }

    void undo_stack::remove(const table_info& table, const primary_key_t pk, variant orig_value) {
        impl_->remove(table, pk, std::move(orig_value));
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
            apply_ = false;
            stack_.apply_changes(revision_);
        }
    }

    void chaindb_session::push() {
        if (apply_) {
            apply_ = false;
            stack_.push(revision_);
        }
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

FC_REFLECT_ENUM(cyberway::chaindb::undo_stage, (Unknown)(New)(Stack)(Rollback))
FC_REFLECT_ENUM(cyberway::chaindb::undo_record, (OldValue)(RemovedValue)(NewValue)(NextPk))