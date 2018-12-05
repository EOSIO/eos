#pragma once

#include <cyberway/chaindb/common.hpp>

namespace cyberway { namespace chaindb {

    using fc::variant;

    class cache_map;
    class driver_interface;
    struct table_info;

    class undo_stack final {
    public:
        undo_stack(driver_interface&, cache_map&);

        undo_stack(const undo_stack&) = delete;
        undo_stack(undo_stack&&) = delete;

        ~undo_stack();

        chaindb_session start_undo_session(bool enabled);

        void set_revision(revision_t revision);
        revision_t revision() const;
        bool enabled() const;

        void apply_changes(revision_t revision);

        /** leaves the UNDO state on the stack when session goes out of scope */
        void push(revision_t revision);

        /**
         *  Restores the state to how it was prior to the current session discarding all changes
         *  made between the last revision and the current revision.
         */
        void undo(revision_t revision);

        /**
         *  This method works similar to git squash, it merges the change set from the two most
         *  recent revision numbers into one revision number (reducing the head revision number)
         *
         *  This method does not change the state of the index, only the state of the undo buffer.
         */
        void squash(revision_t revision);

        /**
         * Discards all undo history prior to revision
         */
        void commit(revision_t revision);

        /**
         * Unwinds all undo states
         */
        void undo_all();
        void undo();

        /**
         * Event on create objects
         */
        void insert(const table_info&, primary_key_t pk);

        /**
         * Event on modify objects
         */
        void update(const table_info&, primary_key_t pk, variant value);

        /**
         * Event on remove objects
         */
        void remove(const table_info&, primary_key_t pk, variant value);

    private:
        struct undo_stack_impl_;
        std::unique_ptr<undo_stack_impl_> impl_;
    }; // class table_undo_stack

} } // namespace cyberway::chaindb