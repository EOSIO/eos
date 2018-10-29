#pragma once

#include <vector>
#include <tuple>
#include <boost/hana.hpp>
#include <functional>
#include <utility>
#include <type_traits>
#include <iterator>
#include <limits>
#include <algorithm>
#include <memory>

#include <boost/multi_index/mem_fun.hpp>
#include <boost/intrusive_ptr.hpp>

#include <eosiolib/action.h>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/chaindb.h>
#include <eosiolib/fixed_key.hpp>

#define chaindb_assert(_EXPR, ...) eosio_assert(_EXPR, __VA_ARGS__)

namespace eosio {

template<typename T, typename MultiIndex>
struct multi_index_item: public T {
    template<typename Constructor>
    multi_index_item(const MultiIndex* midx, Constructor&& constructor)
    : multidx_(midx) {
        constructor(*this);
    }

    const MultiIndex* const multidx_;
    bool deleted_ = false;
    int ref_cnt_ = 0;
}; // struct multi_index_item

template <typename T, typename MultiIndex>
inline void intrusive_ptr_add_ref(eosio::multi_index_item<T, MultiIndex>* obj) {
    ++obj->ref_cnt_;
}

template <typename T, typename MultiIndex>
inline void intrusive_ptr_release(eosio::multi_index_item<T, MultiIndex>* obj) {
    --obj->ref_cnt_;
    if (!obj->ref_cnt_) delete obj;
}

using boost::multi_index::const_mem_fun;

template<index_name_t IndexName, typename Extractor>
struct indexed_by {
    enum constants { index_name = IndexName };
    typedef Extractor extractor_type;
};

struct primary_key_extractor {
    template<typename T>
    primary_key_t operator()(const T& o) const {
        return o.primary_key();
    }
}; // struct primary_key_extractor

template<typename O>
void pack_object(const O& o, char* data, const size_t size) {
    datastream<char*> ds(data, size);
    ds << o;
}

template<typename O>
void unpack_object(O& o, const char* data, const size_t size) {
    datastream<const char*> ds(data, size);
    ds >> o;
}

template<typename Size, typename Lambda>
void safe_allocate(const Size size, const char* error_msg, Lambda&& callback) {
    chaindb_assert(size > 0, error_msg);

    constexpr static size_t max_stack_data_size = 512;

    struct allocator {
        char* data = nullptr;
        const bool use_malloc;
        const size_t size;
        allocator(const size_t s)
            : use_malloc(s > max_stack_data_size), size(s) {
            if (use_malloc) data = static_cast<char*>(malloc(s));
        }

        ~allocator() {
            if (use_malloc) free(data);
        }
    } alloc(static_cast<size_t>(size));

    if (!alloc.use_malloc) alloc.data = static_cast<char*>(alloca(alloc.size));
    chaindb_assert(alloc.data != nullptr, "unable to allocate memory");

    callback(alloc.data, alloc.size);
}

template<table_name_t TableName, typename T, typename... Indices>
class multi_index {
private:
    static_assert(sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices");

    constexpr static bool validate_table_name(table_name_t n) {
        // Limit table names to 12 characters so that
        // the last character (4 bits) can be used to distinguish between the secondary indices.
        return (n & 0x000000000000000FULL) == 0;
    }

    static_assert(
        validate_table_name(TableName),
        "multi_index does not support table names with a length greater than 12");

    using item = multi_index_item<T, multi_index>;
    using item_ptr = boost::intrusive_ptr<item>;
    using primary_key_extractor_type = primary_key_extractor;

    template<index_name_t IndexName, typename Extractor> struct index;

    const account_name_t code_;
    const account_name_t scope_;

    mutable primary_key_t next_primary_key_ = end_primary_key;
    mutable std::vector<item_ptr> items_vector_;

    template<index_name_t IndexName>
    struct const_iterator_impl: public std::iterator<std::bidirectional_iterator_tag, const T> {
    public:
        friend bool operator == (const const_iterator_impl& a, const const_iterator_impl& b) {
            return a.primary_key_ == b.primary_key_;
        }
        friend bool operator != (const const_iterator_impl& a, const const_iterator_impl& b) {
            return !(operator == (a, b));
        }

        constexpr static table_name_t table_name() { return TableName; }
        constexpr static index_name_t index_name() { return IndexName; }
        account_name_t get_code() const            { return multidx_->get_code(); }
        account_name_t get_scope() const           { return multidx_->get_scope(); }

        const T& operator*() const {
            load_object();
            //return *static_cast<const T*>(item_.get());
            return *static_cast<const T*>(item_.get());
        }
        const T* operator->() const {
            load_object();
            // return static_cast<const T*>(item_.get());
            return static_cast<const T*>(item_.get());
        }

        const_iterator_impl operator++(int) {
            const_iterator_impl result(*this);
            ++(*this);
            return result;
        }
        const_iterator_impl& operator++() {
            chaindb_assert(primary_key_ != end_primary_key, "cannot increment end iterator");
            primary_key_ = chaindb_next(get_code(), cursor_);
            item_.reset();
            return *this;
        }

        const_iterator_impl operator--(int) {
            const_iterator_impl result(*this);
            --(*this);
            return result;
        }
        const_iterator_impl& operator--() {
            lazy_load_end_cursor();
            primary_key_ = chaindb_prev(get_code(), cursor_);
            item_.reset();
            chaindb_assert(primary_key_ != end_primary_key, "out of range on decrement of iterator");
            return *this;
        }

        const_iterator_impl() = default;

        const_iterator_impl(const_iterator_impl&& src) {
            this->operator=(std::move(src));
        }
        const_iterator_impl& operator=(const_iterator_impl&& src) {
            if (this == &src) return *this;

            multidx_ = src.multidx_;
            cursor_ = src.cursor_;
            primary_key_ = src.primary_key_;
            item_ = src.item_;

            src.cursor_ = invalid_cursor;
            src.primary_key_ = end_primary_key;
            src.item_.reset();

            return *this;
        }

        const_iterator_impl(const const_iterator_impl& src) {
            this->operator=(src);
        }
        const_iterator_impl& operator=(const const_iterator_impl& src) {
            if (this == &src) return *this;

            multidx_ = src.multidx_;
            cursor_ = chaindb_clone(get_code(), src.cursor_);
            primary_key_ = src.primary_key_;
            item_ = src.item_;

            return *this;
        }

        ~const_iterator_impl() {
            if (cursor_ != invalid_cursor) chaindb_close(get_code(), cursor_);
        }

    private:
        friend multi_index;
        template<index_name_t, typename> struct index;

        const_iterator_impl(const multi_index* midx)
        : multidx_(midx) { }

        const_iterator_impl(const multi_index* midx, const cursor_t cursor)
        : multidx_(midx), cursor_(cursor) {
            if (cursor_ != invalid_cursor) primary_key_ = chaindb_current(get_code(), cursor);
        }

        const_iterator_impl(const multi_index* midx, const cursor_t cursor, const primary_key_t pk, item_ptr item)
        : multidx_(midx), cursor_(cursor), primary_key_(pk), item_(item) { }

        const multi_index* multidx_ = nullptr;
        mutable cursor_t cursor_ = invalid_cursor;
        mutable primary_key_t primary_key_ = end_primary_key;
        mutable item_ptr item_;

        void load_object() const {
            if (item_ && !item_->deleted_) return;

            chaindb_assert(primary_key_ != end_primary_key, "cannot load object from end iterator");
            item_ = multidx_->load_object(cursor_, primary_key_);
        }

        void lazy_load_end_cursor() {
            if (primary_key_ != end_primary_key || cursor_ != invalid_cursor) return;

            cursor_ = chaindb_end(get_code(), get_scope(), table_name(), index_name());
            chaindb_assert(cursor_ != invalid_cursor, "unable to open end iterator");
        }
    }; /// struct multi_index::const_iterator_impl

    template<index_name_t IndexName, typename Extractor>
    struct index {
    public:
        using extractor_type = Extractor;
        using key_type = typename std::decay<decltype(Extractor()(nullptr))>::type;
        using const_iterator = const_iterator_impl<IndexName>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr static bool validate_index_name(index_name_t n) {
           return n != 0;
        }

        static_assert(validate_index_name(IndexName), "invalid index name used in multi_index");

        constexpr static table_name_t table_name() { return TableName; }
        constexpr static index_name_t index_name() { return IndexName; }
        account_name_t get_code() const            { return multidx_->get_code(); }
        account_name_t get_scope() const           { return multidx_->get_scope(); }

        const_iterator cbegin() const {
            auto cursor = chaindb_lower_bound(get_code(), get_scope(), table_name(), index_name(), nullptr, 0);
            return {multidx_, cursor};
        }
        const_iterator begin() const  { return cbegin(); }

        const_iterator cend() const {
            return {multidx_};
        }
        const_iterator end() const  { return cend(); }

        const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
        const_reverse_iterator rbegin() const  { return crbegin(); }

        const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
        const_reverse_iterator rend() const  { return crend(); }

        const_iterator find(key_type&& key) const {
           return find(key);
        }
        const_iterator find(const key_type& key) const {
           auto itr = lower_bound(key);
           auto etr = cend();
           if (itr == etr) return etr;
           if (key != extractor_type()(*itr)) return etr;
           return itr;
        }

        const_iterator require_find(key_type&& key, const char* error_msg = "unable to find key") const {
            return require_find(key, error_msg);
        }
        const_iterator require_find(const key_type& key, const char* error_msg = "unable to find key") const {
            auto itr = lower_bound(key);
            chaindb_assert(itr != cend(), error_msg);
            chaindb_assert(key == extractor_type()(*itr), error_msg);
            return itr;
        }

        const T& get(key_type&& key, const char* error_msg = "unable to find key") const {
            return get(key, error_msg);
        }
        const T& get(const key_type& key, const char* error_msg = "unable to find key") const {
            auto itr = find(key);
            chaindb_assert(itr != cend(), error_msg);
            return *itr;
        }

        const_iterator lower_bound(key_type&& key) const {
           return lower_bound(key);
        }
        const_iterator lower_bound(const key_type& key) const {
            cursor_t cursor;
            safe_allocate(pack_size(key), "invalid size of key", [&](auto& data, auto& size) {
                pack_object(key, data, size);
                cursor = chaindb_lower_bound(get_code(), get_scope(), table_name(), index_name(), data, size);
            });
            return const_iterator(multidx_, cursor);
        }

        const_iterator upper_bound(key_type&& key) const {
           return upper_bound(key);
        }
        const_iterator upper_bound(const key_type& key) const {
            cursor_t cursor;
            safe_allocate(pack_size(key), "invalid size of key", [&](auto& data, auto& size) {
                pack_object(key, data, size);
                cursor = chaindb_upper_bound(get_code(), get_scope(), table_name(), index_name(), data, size);
            });
            return const_iterator(multidx_, cursor);
        }

        const_iterator iterator_to(const T& obj) const {
            const auto& itm = static_cast<const item&>(obj);
            chaindb_assert(itm.multidx_ == multidx_, "object passed to iterator_to is not in multi_index");

            auto key = extractor_type()(itm);
            auto pk = primary_key_extractor_type()(itm);
            cursor_t cursor;
            safe_allocate(pack_size(key), "invalid size of key", [&](auto& data, auto& size) {
                pack_object(key, data, size);
                cursor = chaindb_find(get_code(), get_scope(), table_name(), index_name(), pk, data, size);
            });

            return const_iterator(multidx_, cursor, pk, item_ptr(&itm, false));
        }

        template<typename Lambda>
        void modify(const_iterator itr, account_name_t payer, Lambda&& updater) const {
            chaindb_assert(itr != cend(), "cannot pass end iterator to modify");
            multidx_->modify(*itr, payer, std::forward<Lambda&&>(updater));
        }

        const_iterator erase(const_iterator itr) const {
            chaindb_assert(itr != cend(), "cannot pass end iterator to erase");
            const auto& obj = *itr;
            ++itr;
            multidx_->erase(obj);
            return itr;
        }

        static auto extract_key(const T& obj) { return extractor_type()(obj); }

    private:
        friend class multi_index;

        index(const multi_index* midx)
        : multidx_(midx) { }

        const multi_index* const multidx_;
    }; /// struct multi_index::index

    using indices_type = decltype(boost::hana::tuple<Indices...>());

    indices_type indices_;

    index<N(primary), primary_key_extractor> primary_idx_;

    item_ptr find_object_in_cache(const primary_key_t pk) const {
        auto itr = std::find_if(items_vector_.rbegin(), items_vector_.rend(), [&pk](const auto& itm) {
            return primary_key_extractor_type()(*itm) == pk;
        });
        if (items_vector_.rend() != itr) {
            return (*itr);
        }
        return item_ptr();
    }

    void add_object_to_cache(item_ptr ptr) const {
        items_vector_.push_back(std::move(ptr));
    }

    void remove_object_from_cache(const primary_key_t pk) const {
        auto itr = std::find_if(items_vector_.rbegin(), items_vector_.rend(), [&pk](const auto& itm) {
            return primary_key_extractor_type()(*itm) == pk;
        });
        if (items_vector_.rend() != itr) {
            (*itr)->deleted_ = true;
            items_vector_.erase(--(itr.base()));
        }
    }

    item_ptr load_object(const cursor_t cursor, const primary_key_t pk) const {
        auto ptr = find_object_in_cache(pk);
        if (ptr) return ptr;

        auto size = chaindb_datasize(get_code(), cursor);

        safe_allocate(size, "invalid unpack object size", [&](auto& data, auto& datasize) {
            auto dpk = chaindb_data(get_code(), cursor, data, datasize);
            chaindb_assert(dpk == pk, "invalid packet object");
            ptr = item_ptr(new item(this, [&](auto& itm) {
                T& obj = static_cast<T&>(itm);
                unpack_object(obj, data, datasize);
            }));
        });

        auto ptr_pk = primary_key_extractor_type()(*ptr);
        chaindb_assert(ptr_pk == pk, "invalid primary key of object");
        add_object_to_cache(ptr);
        return ptr;
    }
public:
    using const_iterator = const_iterator_impl<N(primary)>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    multi_index(const account_name_t code, account_name_t scope)
    : code_(code), scope_(scope), primary_idx_(this) {}

    constexpr static table_name_t table_name() { return TableName; }

    account_name_t get_code() const  { return code_; }
    account_name_t get_scope() const { return scope_; }

    const_iterator cbegin() const { return primary_idx_.cbegin(); }
    const_iterator begin() const  { return cbegin(); }

    const_iterator cend() const  { return primary_idx_.cend(); }
    const_iterator end() const   { return cend(); }

    const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
    const_reverse_iterator rbegin() const  { return crbegin(); }

    const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }
    const_reverse_iterator rend() const  { return crend(); }

    const_iterator lower_bound(primary_key_t pk) const {
        return primary_idx_.lower_bound(pk);
    }

    const_iterator upper_bound(primary_key_t pk) const {
        return primary_idx_.upper_bound(pk);
    }

    primary_key_t available_primary_key() const {
        if (next_primary_key_ == end_primary_key) {
            next_primary_key_ = chaindb_available_primary_key(get_code(), get_scope(), table_name());
            chaindb_assert(next_primary_key_ != end_primary_key, "no available primary key");
        }
        return next_primary_key_;
    }

    template<index_name_t IndexName>
    auto get_index() const {
        namespace hana = boost::hana;

        auto res = hana::find_if(indices_, [](auto&& in) {
            return std::integral_constant<
                bool, std::decay<decltype(in)>::type::index_name == IndexName>();
        });

        static_assert(res != hana::nothing, "name provided is not the name of any secondary index within multi_index");

        return index<IndexName, typename std::decay<decltype(res.value())>::type::extractor_type>(this);
    }

    const_iterator iterator_to(const T& obj) const {
        return primary_idx_.iterator_to(obj);
    }

    template<typename Lambda>
    const_iterator emplace(const account_name_t payer, Lambda&& constructor) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
        chaindb_assert(get_code() == current_receiver(), "cannot modify objects in table of another contract");

        auto ptr = item_ptr(new item(this, [&](auto& itm) {
            constructor(static_cast<T&>(itm));
        }));

        auto& obj = static_cast<T&>(*ptr);
        auto pk = primary_key_extractor_type()(obj);
        chaindb_assert(pk != end_primary_key, "invalid value of primary key");

        cursor_t cursor;
        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            cursor = chaindb_insert(get_code(), get_scope(), table_name(), payer, pk, data, size);
        });

        chaindb_assert(cursor != invalid_cursor, "unable to create object");

        add_object_to_cache(ptr);

        next_primary_key_ = pk + 1;
        return const_iterator(this, cursor, pk, std::move(ptr));
    }

    template<typename Lambda>
    void modify(const_iterator itr, const account_name_t payer, Lambda&& updater) const {
        chaindb_assert(itr != end(), "cannot pass end iterator to modify");
        modify(*itr, payer, std::forward<Lambda&&>(updater));
    }

    template<typename Lambda>
    void modify(const T& obj, const account_name_t payer, Lambda&& updater) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
        chaindb_assert(get_code() == current_receiver(), "cannot modify objects in table of another contract");

        const auto& itm = static_cast<const item&>(obj);
        chaindb_assert(itm.multidx_ == this, "object passed to modify is not in multi_index");

        auto pk = primary_key_extractor_type()(obj);

        auto& mobj = const_cast<T&>(obj);
        updater(mobj);

        auto mpk = primary_key_extractor_type()(obj);
        chaindb_assert(pk == mpk, "updater cannot change primary key when modifying an object");

        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            auto upk = chaindb_update(get_code(), get_scope(), table_name(), payer, pk, data, size);
            chaindb_assert(upk == pk, "unable to update object");
        });
    }

    const T& get(const primary_key_t pk, const char* error_msg = "unable to find key") const {
        auto itr = find(pk);
        chaindb_assert(itr != cend(), error_msg);
        return *itr;
    }

    const_iterator find(const primary_key_t pk) const {
        return primary_idx_.find(pk);
    }

    const_iterator require_find(const primary_key_t pk, const char* error_msg = "unable to find key") const {
        return primary_idx_.require_find(pk, error_msg);
    }

    const_iterator erase(const_iterator itr) const {
        chaindb_assert(itr != end(), "cannot pass end iterator to erase");

        const auto& obj = *itr;
        ++itr;
        erase(obj);
        return itr;
    }

    void erase(const T& obj) const {
        const auto& itm = static_cast<const item&>(obj);
        chaindb_assert(itm.multidx_ == this, "object passed to erase is not in multi_index");

        auto pk = primary_key_extractor_type()(obj);
        remove_object_from_cache(pk);
        auto dpk = chaindb_delete(get_code(), get_scope(), table_name(), pk);
        chaindb_assert(dpk == pk, "unable to delete object");
    }
}; // class multi_index
}  // namespace eosio