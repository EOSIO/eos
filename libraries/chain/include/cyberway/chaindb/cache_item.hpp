#pragma once

#include <memory>

#include <eosio/chain/types.hpp>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

namespace cyberway { namespace chaindb {

    using  eosio::chain::bytes;

    class cache_map_impl;
    class cache_object;
    using cache_object_ptr = boost::intrusive_ptr<cache_object>;

    struct cache_data {
        virtual ~cache_data() = default;
    }; // struct cache_data

    using cache_data_ptr = std::unique_ptr<cache_data>;

    struct cache_converter_interface {
        cache_converter_interface() = default;
        virtual ~cache_converter_interface() = default;

        virtual void convert_variant(cache_object&, const object_value&) const = 0;
    }; // struct cache_converter_interface

    struct cache_index_value final: public boost::intrusive::set_base_hook<> {
        const index_name index;
        const bytes      blob;
        const cache_object* const object = nullptr;

        cache_index_value(index_name, bytes, const cache_object&);
        cache_index_value(cache_index_value&&) = default;

        ~cache_index_value() = default;
    }; // struct cache_index_value

    using cache_indicies = std::vector<cache_index_value>;

    struct cache_cell {
        enum cache_cell_kind {
            Unknown,
            Pending,
            LRU,
            System,
        }; // enum cell_kind

        cache_map_impl* const map  = nullptr;
        const cache_cell_kind kind = Unknown;

        uint64_t pos  = 0;
        uint64_t size = 0;

        cache_cell(cache_map_impl& m, cache_cell_kind k)
        : map(&m), kind(k) {
        }
    }; // struct cache_cell

    struct cache_object_state {
        cache_cell* const cell = nullptr;
        cache_object_ptr  object_ptr;

        cache_object_state(cache_cell& c, cache_object_ptr ptr)
        : cell(&c), object_ptr(std::move(ptr)) {
        }

        void reset();
    }; // struct cache_object_state

    class cache_object final:
        public boost::intrusive_ref_counter<cache_object>,
        public boost::intrusive::set_base_hook<>
    {
        cache_object_state* state_ = nullptr;
        object_value        object_;
        bytes               blob_;  // for contracts tables
        cache_data_ptr      data_;  // for interchain tables
        cache_indicies      indicies_;

    private:
        friend class  cache_map_impl;
        friend struct lru_cache_cell;
        friend struct system_cache_cell;
        friend struct pending_cache_cell;
        friend struct pending_cache_object_state;

        const cache_cell&   cell() const;
        cache_cell&         cell();
        cache_map_impl&     map();
        cache_object_state& state();
        cache_object_state* swap_state(cache_object_state& state);

    public:
        cache_object() = default;
        cache_object(cache_object&&) = default;

        ~cache_object() = default;

        bool is_valid_cell() const;
        bool has_cell() const;
        bool is_same_cell(const cache_cell& cell) const;

        bool is_deleted() const;

        template <typename Request>
        bool is_valid_table(const Request& request) const {
            return
                object_.service.code  == request.code  &&
                object_.service.scope == request.scope &&
                object_.service.table == request.table;
        }

        void mark_deleted();
        void release();

        void set_object(object_value);
        void set_service(service_state service);

        void set_revision(const revision_t rev) {
            object_.service.revision = rev;
        }

        template <typename T, typename... Args> void set_data(Args&&... args) {
            data_ = std::make_unique<T>(*this, std::forward<Args>(args)...);
        }

        void set_blob(bytes blob) {
            blob_ = std::move(blob);
        }

        primary_key_t pk() const {
            return object_.pk();
        }

        const object_value& object() const {
            return object_;
        }

        const service_state& service() const {
            return object_.service;
        }

        bool has_data() const {
            return !!data_;
        }

        template <typename T> T& data() const {
            assert(has_data());
            return *static_cast<T*>(data_.get());
        }

        bool has_blob() const {
            return !blob_.empty();
        }

        const bytes& blob() const {
            assert(has_blob());
            return blob_;
        }

    }; // class cache_object

} } // namespace cyberway::chaindb
