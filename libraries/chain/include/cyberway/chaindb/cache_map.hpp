#pragma once

#include <cyberway/chaindb/cache_item.hpp>
#include <cyberway/chaindb/storage_payer_info.hpp>

namespace cyberway { namespace chaindb {

    struct table_info;
    struct index_info;
    struct object_value;

    class cache_map final {
    public:
        cache_map(abi_map&);
        ~cache_map();

        void set_cache_converter(const table_info&, const cache_converter_interface&) const;

        void set_next_pk(const table_info&, primary_key_t) const;

        cache_object_ptr create(const table_info&, primary_key_t, const storage_payer_info&) const;
        cache_object_ptr create(const table_info&, const storage_payer_info&) const;
        cache_object_ptr find(const table_info&, primary_key_t) const;
        cache_object_ptr find(const index_info&, const char*, size_t) const;

        cache_object_ptr emplace(const table_info&, object_value) const;
        void remove(const table_info&, primary_key_t) const;
        void set_revision(const object_value&, revision_t) const;

        uint64_t calc_ram_bytes(revision_t) const;
        void set_revision(revision_t) const;
        void start_session(revision_t) const;
        void push_session(revision_t) const;
        void squash_session(revision_t) const;
        void undo_session(revision_t) const;

        void clear() const;

    private:
        std::unique_ptr<cache_map_impl> impl_;
    }; // class cache_map

} } // namespace cyberway::chaindb
