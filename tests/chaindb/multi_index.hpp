#pragma once

#include <string.h>

#include <fc/io/datastream.hpp>

#include <eosio/chain/config.hpp>

#include <vector>
#include <tuple>
#include <functional>
#include <utility>
#include <type_traits>
#include <iterator>
#include <limits>
#include <algorithm>
#include <memory>

#include <boost/hana.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "chaindb.h"

using fc::raw::pack_size;

#define chaindb_assert(_EXPR, ...) EOS_ASSERT(_EXPR, fc::exception, __VA_ARGS__)

namespace boost { namespace tuples {

template<typename T,typename I1>
fc::datastream<T>& operator<<(fc::datastream<T>& stream, const boost::tuple<I1>& value) {
    // TODO Do streaming tuple values
    fc::raw::pack(stream, boost::tuples::get<0>(value));
    return stream;
}

template<typename T,typename I1, typename I2>
fc::datastream<T>& operator<<(fc::datastream<T>& stream, const boost::tuple<I1,I2>& value) {
    // TODO Do streaming tuple values
    fc::raw::pack(stream, boost::tuples::get<0>(value));
    fc::raw::pack(stream, boost::tuples::get<1>(value));
    return stream;
}

template<typename T,typename I1, typename I2, typename I3>
fc::datastream<T>& operator<<(fc::datastream<T>& stream, const boost::tuple<I1,I2,I3>& value) {
    // TODO Do streaming tuple values
    fc::raw::pack(stream, boost::tuples::get<0>(value));
    fc::raw::pack(stream, boost::tuples::get<1>(value));
    fc::raw::pack(stream, boost::tuples::get<2>(value));
    return stream;
}

} } // namespace boost::tuples

namespace chaindb {

inline const uint64_t hash_64_fnv1a(const void* key, const uint64_t len) {
    
    const char* data = (char*)key;
    uint64_t hash = 0xcbf29ce484222325;
    uint64_t prime = 0x100000001b3;
    
    for(int i = 0; i < len; ++i) {
        uint8_t value = data[i];
        hash = hash ^ value;
        hash *= prime;
    }
    
    return hash;

} //hash_64_fnv1a


inline const uint64_t hash_64_fnv1a(const std::string& value) {
    return hash_64_fnv1a(value.c_str(), value.length());
}

using boost::multi_index::const_mem_fun;

template<index_name_t IndexName, typename Extractor>
struct indexed_by {
    //enum constants { index_name = IndexName };
    typedef Extractor extractor_type;
    using index_name = std::integral_constant<index_name_t,IndexName>;
};

struct primary_key_extractor {
    template<typename T>
    primary_key_t operator()(const T& o) const {
        return o.primary_key();
    }
}; // struct primary_key_extractor

template<typename O>
void pack_object(const O& o, char* data, const size_t size) {
    fc::datastream<char*> ds(data, size);
    fc::raw::pack(ds, o);
}

template<typename O>
void unpack_object(O& o, const char* data, const size_t size) {
    fc::datastream<const char*> ds(data, size);
    fc::raw::unpack(ds, o);
}

template<typename Lambda>
void safe_allocate(const size_t size, const char* error_msg, Lambda&& callback) {
    chaindb_assert(size > 0, error_msg);

    constexpr static size_t max_stack_data_size = 512;

    struct allocator {
        char* data = nullptr;
        const bool use_malloc;
        allocator(const size_t s)
            : use_malloc(s > max_stack_data_size) {
            if (use_malloc) data = static_cast<char*>(malloc(s));
        }

        ~allocator() {
            if (use_malloc) free(data);
        }
    } alloc(size);

    if (!alloc.use_malloc) alloc.data = static_cast<char*>(alloca(size));
    chaindb_assert(alloc.data != nullptr, "unable to allocate memory");

    callback(alloc.data, size);
}

template<class C> struct tag {
    using type = C;

    static uint64_t get_name() {
        return hash_64_fnv1a(boost::core::demangle(typeid(C).name()));
    }
};

template<typename C> struct default_comparator {
};

template<typename Tag, typename KeyExtractor, typename KeyComparator = default_comparator<typename KeyExtractor::result_type> > 
struct ordered_unique {
    using tag_type = typename Tag::type;
    using extractor_type = KeyExtractor;
    using comparator_type = KeyComparator;
};

template<typename Key>
struct key_comparator {
    static bool compare_eq(const Key& left, const Key& right) {
        return left == right;
    }
};

template<typename... Indices>
struct key_comparator<boost::tuple<Indices...>> {
    using key_type = boost::tuple<Indices...>;

    static bool compare_eq(const key_type& left, const key_type& right) {
        return left == right;
    }

    template<typename Value>
    static bool compare_eq(const key_type& left, const Value& value) {
        return boost::tuples::get<0>(left) == value;
    }

    template<typename V1,typename V2>
    static bool compare_eq(const key_type& left, const boost::tuple<V1,V2>& value) {
        return boost::tuples::get<0>(left) == boost::tuples::get<0>(value) &&
               boost::tuples::get<1>(left) == boost::tuples::get<1>(value);
    }
};

template<typename Key>
struct key_converter {
    static Key convert(const Key& key) {
        return key;
    }
};

template<typename... Indices>
struct key_converter<boost::tuple<Indices...>> {
    template<typename Value>
    static boost::tuple<Indices...> convert(const Value& value) {
        boost::tuple<Indices...> index;
        boost::tuples::get<0>(index) = value;
        return index;
    }

    template<typename Value>
    static boost::tuple<Indices...> convert(const boost::tuple<Value>& value) {
        boost::tuple<Indices...> index;
        boost::tuples::get<0>(index) = boost::tuples::get<0>(value);
        return index;
    }

    template<typename V1,typename V2>
    static boost::tuple<Indices...> convert(const boost::tuple<V1,V2>& value) {
        boost::tuple<Indices...> index;
        boost::tuples::get<0>(index) = boost::tuples::get<0>(value);
        boost::tuples::get<1>(index) = boost::tuples::get<1>(value);
        return index;
    }
};


template<typename T>
struct name_extractor;

template<typename C>
struct name_extractor<tag<C>> {
    static uint64_t get_name() {
        return tag<C>::get_name();
    }
};

template<uint64_t N>
struct name_extractor<std::integral_constant<uint64_t,N>> {
    constexpr static uint64_t get_name() {return N;}
};

template<typename Tag, typename Extractor>
struct PrimaryIndex {
    using tag = Tag;
    using extractor = Extractor;
};

template<typename TableName, typename Index, typename T, typename Allocator, typename... Indices>
struct multi_index_impl {
//private:
    static_assert(sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices");

    const account_name_t code_;
    const account_name_t scope_;
    Allocator allocator_;

    mutable primary_key_t next_primary_key_ = unset_primary_key;

    struct item: public T {
        /*template<typename Constructor>
        item(const multi_index_impl* midx, Constructor&& constructor)
            : multidx_(midx) {
            constructor(*this);
        }*/
    
    
    
        template<typename Constructor>
        item(const multi_index_impl* midx, Constructor&& constructor)
            : T(constructor, midx->allocator_), multidx_(midx) 
        {
        }

        const multi_index_impl* const multidx_;
        bool deleted_ = false;
    
        using value_type = T;
    }; // struct item

    using item_ptr = std::shared_ptr<item>;

    mutable std::vector<item_ptr> items_vector_;

    using primary_key_extractor_type = typename Index::extractor;

    template<typename IndexName, typename Extractor> struct index;

    template<typename IndexName>
    struct const_iterator_impl: public std::iterator<std::bidirectional_iterator_tag, const T> {
    public:
        friend bool operator == (const const_iterator_impl& a, const const_iterator_impl& b) {
            return a.primary_key_ == b.primary_key_;
        }
        friend bool operator != (const const_iterator_impl& a, const const_iterator_impl& b) {
            return !(operator == (a, b));
        }

        constexpr static table_name_t table_name() { return name_extractor<TableName>::get_name(); }
        constexpr static index_name_t index_name() { return name_extractor<IndexName>::get_name(); }
        account_name_t get_code() const            { return multidx_->get_code(); }
        account_name_t get_scope() const           { return multidx_->get_scope(); }

        const T& operator*() const {
            load_object();
            return *static_cast<const T*>(item_.get());
        }
        const T* operator->() const {
            load_object();
            return static_cast<const T*>(item_.get());
        }

        const_iterator_impl operator++(int) {
            const_iterator_impl result(*this);
            ++(*this);
            return result;
        }
        const_iterator_impl& operator++() {
            chaindb_assert(primary_key_ != unset_primary_key, "cannot increment end iterator");
            primary_key_ = chaindb_next(cursor_);
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
            primary_key_ = chaindb_prev(cursor_);
            item_.reset();
            chaindb_assert(primary_key_ != unset_primary_key, "out of range on decrement of iterator");
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
            src.primary_key_ = unset_primary_key;
            src.item_.reset();

            return *this;
        }

        const_iterator_impl(const const_iterator_impl& src) {
            this->operator=(src);
        }
        const_iterator_impl& operator=(const const_iterator_impl& src) {
            if (this == &src) return *this;

            multidx_ = src.multidx_;
            cursor_ = chaindb_clone(src.cursor_);
            primary_key_ = src.primary_key_;
            item_ = src.item_;

            return *this;
        }

        ~const_iterator_impl() {
            if (cursor_ != invalid_cursor) chaindb_close(cursor_);
        }

    //private:
        friend multi_index_impl;
        template<index_name_t, typename> struct index;

        const_iterator_impl(const multi_index_impl* midx)
            : multidx_(midx) { }

        const_iterator_impl(const multi_index_impl* midx, const cursor_t cursor)
            : multidx_(midx), cursor_(cursor) {
            if (cursor_ != invalid_cursor) primary_key_ = chaindb_current(cursor);
        }

        const_iterator_impl(const multi_index_impl* midx, const cursor_t cursor, const primary_key_t pk)
            : multidx_(midx), cursor_(cursor), primary_key_(pk) {}

        const multi_index_impl* multidx_ = nullptr;
        mutable cursor_t cursor_ = invalid_cursor;
        mutable primary_key_t primary_key_ = unset_primary_key;
        mutable item_ptr item_;

        void load_object() const {
            chaindb_assert(!item_ || !item_->deleted_, "cannot load deleted object");
            if (item_) return;

            chaindb_assert(primary_key_ != unset_primary_key, "cannot load object from end iterator");
            item_ = multidx_->load_object(cursor_, primary_key_);
        }

        void lazy_load_end_cursor() {
            if (primary_key_ != unset_primary_key || cursor_ != invalid_cursor) return;

            cursor_ = chaindb_end(get_code(), get_scope(), table_name(), index_name());
            chaindb_assert(cursor_ != invalid_cursor, "unable to open end iterator");
        }
    }; /// struct multi_index_impl::const_iterator_impl

    template<typename IndexName, typename Extractor>
    struct index {
    public:
        using extractor_type = Extractor;
        using key_type = typename std::decay<decltype(Extractor()(static_cast<const T&>(*(const T*)nullptr)))>::type;
        using const_iterator = const_iterator_impl<IndexName>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr static table_name_t table_name() { return name_extractor<TableName>::get_name(); }
        constexpr static index_name_t index_name() { return name_extractor<IndexName>::get_name(); }
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
        
        template<typename Value>
        const_iterator find(const Value& value) const {
           auto key = key_converter<key_type>::convert(value);
           auto itr = lower_bound(key);
           auto etr = cend();
           if (itr == etr) return etr;
           if (!key_comparator<key_type>::compare_eq(extractor_type()(*itr), value)) return etr;
           return itr;
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

        template<typename Value>
        const_iterator lower_bound(const Value& value) const {
            return lower_bound(key_converter<key_type>::convert(value));
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

        std::pair<const_iterator,const_iterator> equal_range(key_type&& key) const {
            return upper_bound(key);
        }
        std::pair<const_iterator,const_iterator> equal_range(const key_type& key) const {
            return make_pair(lower_bound(key), upper_bound(key));
        }

        template<typename Value>
        std::pair<const_iterator,const_iterator> equal_range(const Value& value) const {
            const_iterator lower = lower_bound(value);
            const_iterator upper = lower;
            auto etr = cend();
            while(upper != etr && key_comparator<key_type>::compare_eq(extractor_type()(*upper), value))
                ++upper;

            return make_pair(lower, upper);
        }

        const_iterator iterator_to(const T& obj) const {
            const auto& itm = static_cast<const item&>(obj);
            chaindb_assert(itm.multidx_ == multidx_, "object passed to iterator_to is not in multi_index");

            auto key = extractor_type()(itm);
            auto pk = primary_key_extractor_type()(obj);
            cursor_t cursor;
            safe_allocate(pack_size(key), "invalid size of key", [&](auto& data, auto& size) {
                pack_object(key, data, size);
                cursor = chaindb_find(get_code(), get_scope(), table_name(), index_name(), pk, data, size);
            });
            return const_iterator(multidx_, cursor, pk);
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

    //private:
        friend struct multi_index_impl;

        index(const multi_index_impl* midx)
            : multidx_(midx) { }

        const multi_index_impl* const multidx_;
    }; /// struct multi_index_impl::index

    template<typename... Idx>
    struct index_container {
        using type = decltype(boost::hana::tuple<Idx...>());
    };

    using indices_type = typename index_container<Indices...>::type; //decltype(boost::hana::tuple<Indices...>());

    indices_type indices_;

    using primary_index_type = index<typename Index::tag,typename Index::extractor>;
    primary_index_type primary_idx_;
    //index<std::integral_constant<index_name_t,N(primary)>, PrimaryKeyExtractor> primary_idx_;
    
    item_ptr find_object_in_cache(const primary_key_t pk) const {
        auto itr = std::find_if(items_vector_.rbegin(), items_vector_.rend(), [&pk](const auto& itm) {
            auto& obj = static_cast<T&>(*itm);
            return primary_key_extractor_type()(obj) == pk;
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
        auto itr = std::find_if(items_vector_.begin(), items_vector_.end(), [&pk](const auto& itm) {
            auto& obj = static_cast<T&>(*itm);
            return primary_key_extractor_type()(obj) == pk;
        });
        if (items_vector_.end() != itr) {
            (*itr)->deleted_ = true;
            items_vector_.erase(itr);
        }
    }

    item_ptr load_object(const cursor_t cursor, const primary_key_t pk) const {
        auto ptr = find_object_in_cache(pk);
        if (ptr) return ptr;

        auto size = chaindb_datasize(cursor);

        safe_allocate(size, "invalid unpack object size", [&](auto& data, auto& size) {
            auto dpk = chaindb_data(cursor, data, size);
            chaindb_assert(dpk == pk, "invalid packet object");
            ptr = std::make_shared<item>(this, [&](auto& itm) {
                T& obj = static_cast<T&>(itm);
                unpack_object(obj, data, size);
            });
        });

        auto& obj = static_cast<T&>(*ptr);
        auto ptr_pk = primary_key_extractor_type()(obj);
        chaindb_assert(ptr_pk == pk, "invalid primary key of object");
        add_object_to_cache(ptr);
        return ptr;
    }
public:
    using const_iterator = typename primary_index_type::const_iterator; //const_iterator_impl<std::integral_constant<index_name_t,N(primary)>>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    multi_index_impl(const account_name_t code, account_name_t scope, Allocator *allocator = nullptr)
        : code_(code), scope_(scope), allocator_(*allocator), primary_idx_(this) {}

    constexpr static table_name_t table_name() { return name_extractor<TableName>::get_name(); }

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
        if (next_primary_key_ == unset_primary_key) {
            next_primary_key_ = chaindb_available_primary_key(get_code(), get_scope(), table_name());
            chaindb_assert(next_primary_key_ != unset_primary_key, "no available primary key");
        }
        return next_primary_key_;
    }

    template<typename IndexTag>
    auto get_index() const {
        namespace hana = boost::hana;

        auto res = hana::find_if(indices_, [](auto&& in) {
            return std::is_same<typename std::decay<decltype(in)>::type::tag_type,tag<IndexTag>>();
        });

        static_assert(res != hana::nothing, "name provided is not the name of any secondary index within multi_index");

        return index<IndexTag, typename std::decay<decltype(res.value())>::type::extractor_type>(this);
    }

    template<index_name_t IndexName>
    auto get_index() const {
        namespace hana = boost::hana;

        auto res = hana::find_if(indices_, [](auto&& in) {
            return std::integral_constant<
                bool, std::decay<decltype(in)>::type::index_name::value == IndexName>();
        });

        static_assert(res != hana::nothing, "name provided is not the name of any secondary index within multi_index");

        return index<std::integral_constant<index_name_t,IndexName>, typename std::decay<decltype(res.value())>::type::extractor_type>(this);
    }

    const_iterator iterator_to(const T& obj) const {
        return primary_idx_.iterator_to(obj);
    }

    template<typename Lambda>
    const_iterator emplace(const account_name_t payer, Lambda&& constructor) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
//        chaindb_assert(
//            get_code() == current_receiver(),
//            "cannot modify objects in table of another contract");

        auto ptr = std::make_unique<item>(this, [&](auto& itm) {
            constructor(static_cast<T&>(itm));
        });

        auto& obj = static_cast<T&>(*ptr);
        auto pk = primary_key_extractor_type()(obj);
        chaindb_assert(pk != unset_primary_key, "invalid value of primary key");

        cursor_t cursor;
        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            cursor = chaindb_insert(get_code(), get_scope(), payer, table_name(), pk, data, size);
        });

        chaindb_assert(cursor != invalid_cursor, "unable to create object");

        next_primary_key_ = pk + 1;
        return const_iterator(this, cursor, pk);
    }

    template<typename Lambda>
    void modify(const_iterator itr, const account_name_t payer, Lambda&& updater) const {
        chaindb_assert(itr != end(), "cannot pass end iterator to modify");
        modify(*itr, payer, std::forward<Lambda&&>(updater));
    }

    template<typename Lambda>
    void modify(const T& obj, const account_name_t payer, Lambda&& updater) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
//        chaindb_assert(
//            get_code() == current_receiver(),
//            "cannot modify objects in table of another contract");

        const auto& itm = static_cast<const item&>(obj);
        chaindb_assert(itm.multidx_ == this, "object passed to modify is not in multi_index");

        auto pk = primary_key_extractor_type()(obj);

        auto& mobj = const_cast<T&>(obj);
        updater(mobj);

        auto mpk = primary_key_extractor_type()(obj);
        if (mpk != pk) {
            remove_object_from_cache(pk);
            chaindb_assert(pk == mpk, "updater cannot change primary key when modifying an object");
        }

        cursor_t cursor;
        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            auto upk = chaindb_update(get_code(), get_scope(), payer, table_name(), pk, data, size);
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
        chaindb_assert(itm.multidx_ == this, "object passed to modify is not in multi_index");

        auto pk = primary_key_extractor_type()(obj);
        remove_object_from_cache(pk);
        auto dpk = chaindb_delete(get_code(), get_scope(), table_name(), pk);
        chaindb_assert(dpk == pk, "unable to delete object");
    }
};

template<table_name_t TableName, typename PrimaryKeyExtractor, typename T, typename... Indices>
using multi_index = multi_index_impl<
    std::integral_constant<table_name_t,TableName>,
    PrimaryIndex<std::integral_constant<index_name_t,N(primary)>,PrimaryKeyExtractor>,
    T,
    Indices...>;

}  /// chaindb
