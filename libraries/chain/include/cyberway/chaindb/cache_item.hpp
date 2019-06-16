#pragma once

#include <memory>
#include <set>
#include <deque>

#include <eosio/chain/types.hpp>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>

namespace cyberway { namespace chaindb {

    class cache_map_impl;

    struct cache_data {
        virtual ~cache_data() = default;
    }; // struct cache_data

    using cache_data_ptr = std::unique_ptr<cache_data>;

    struct cache_converter_interface {
        cache_converter_interface() = default;
        virtual ~cache_converter_interface() = default;

        virtual void convert_variant(cache_object&, const object_value&) const = 0;
    }; // struct cache_converter_interface

    struct cache_cell {
        enum cache_cell_kind {
            Unknown,
            Pending,
            LRU,
            System,
        }; // enum cell_kind

        cache_map_impl* const map  = nullptr;

        uint64_t size = 0;

        cache_cell(cache_map_impl& m, uint64_t p, cache_cell_kind k)
        : map(&m), pos_(p), kind_(k) {
        }

        cache_cell_kind kind() const {
            return kind_;
        }

        uint64_t pos() const {
            return pos_;
        }

    protected:
        const uint64_t pos_ = 0;
        cache_cell_kind kind_ = Unknown;
    }; // struct cache_cell

    struct cache_object_state {
        cache_cell* const cell = nullptr;
        cache_object_ptr  object_ptr;

        cache_object_state(cache_cell& c, cache_object_ptr ptr)
        : cell(&c), object_ptr(std::move(ptr)) {
        }

        void reset();
        cache_cell::cache_cell_kind kind() const;
    }; // struct cache_object_state

    struct cache_index_value;
    struct cache_index_key;

    struct cache_index_compare final {
        using is_transparent = void;
        template<typename LeftKey, typename RightKey> bool operator()(const LeftKey&, const RightKey&) const;
    }; // struct cache_index_compare

    using cache_index_tree = std::set<cache_index_value, cache_index_compare>;
    using cache_indicies   = std::deque<cache_index_tree::const_iterator>;

    struct cache_object final: public boost::intrusive_ref_counter<cache_object>
    {
        enum stage_kind {
            Released,
            Active,
            Deleted,
        }; // enum Stage

        cache_object(object_value);

        cache_object(cache_object&&) = delete;
        cache_object(const cache_object&) = delete;

        ~cache_object() = default;

        bool is_valid_cell() const;
        bool has_cell() const;
        bool is_same_cell(const cache_cell& cell) const;

        stage_kind stage() const {
            return stage_;
        }

        bool is_deleted() const {
            return stage_ != Active;
        }

        template <typename Request> bool is_valid_table(const Request& request) const {
            return
                object_.service.code  == request.code  &&
                object_.service.scope == request.scope &&
                object_.service.table == request.table;
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

    private:
        cache_object_state* state_ = nullptr;
        object_value        object_;
        bytes               blob_;  // for contracts tables
        cache_data_ptr      data_;  // for interchain tables
        cache_indicies      indicies_;
        stage_kind          stage_ = Released;

        friend class  cache_map_impl;
        friend struct lru_cache_cell;
        friend struct lru_cache_object_state;
        friend struct system_cache_cell;

        using cache_cell_kind = cache_cell::cache_cell_kind;

        cache_cell&         cell() const;
        cache_cell_kind     kind() const;
        cache_map_impl&     map() const;
        cache_object_state& state() const;
        cache_object_state* swap_state(cache_object_state& state);
    }; // class cache_object

    struct cache_object_key;

    using cache_object_tree = std::map<cache_object_key, cache_object_ptr>;

} } // namespace cyberway::chaindb
