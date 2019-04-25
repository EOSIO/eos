#pragma once

#include "action.h"
#include "types.hpp"
#include "serialize.hpp"
#include "datastream.hpp"
#include "chaindb.h"
#include "fixed_key.hpp"

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

#define chaindb_assert(_EXPR, ...) eosio_assert(_EXPR, __VA_ARGS__)

#ifndef CHAINDB_ANOTHER_CONTRACT_PROTECT
#  define CHAINDB_ANOTHER_CONTRACT_PROTECT(_CHECK, _MSG) \
     chaindb_assert(_CHECK, _MSG)
#endif // CHAINDB_ANOTHER_CONTRACT_PROTECT


namespace eosio {

template<class Class,typename Type,Type (Class::*PtrToMemberFunction)()const>
struct const_mem_fun {
  typedef typename std::remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

  auto operator()(const ChainedPtr& x)const -> std::enable_if_t<!std::is_convertible<const ChainedPtr&, const Class&>::value, Type> {
    return operator()(*x);
  }

  Type operator()(const Class& x)const {
    return (x.*PtrToMemberFunction)();
  }

  Type operator()(const std::reference_wrapper<const Class>& x)const {
    return operator()(x.get());
  }

  Type operator()(const std::reference_wrapper<Class>& x)const {
    return operator()(x.get());
  }
};

template<typename A>
struct get_result_type {
    using type = typename A::result_type;
}; // struct get_result_type

template<typename Value, typename... Fields>
struct composite_key {
    using result_type = bool;
    using key_type = std::tuple<typename get_result_type<Fields>::type...>;

    auto operator()(const Value& v) {
        return std::make_tuple(Fields()(v)...);
    }
}; // struct composite_key

namespace _detail {
template<class Class,typename Type,Type Class::*PtrToMember>
struct const_member_base {
    typedef Type result_type;

    template<typename ChainedPtr>
    auto operator()(const ChainedPtr& x) const
    -> std::enable_if_t<!std::is_convertible<const ChainedPtr&, const Class&>::type, Type&> {
        return operator()(*x);
    }

    Type& operator()(const Class& x) const {
        return x.*PtrToMember;
    }
    Type& operator()(const std::reference_wrapper<const Class>& x) const {
        return operator()(x.get());
    }
    Type& operator()(const std::reference_wrapper<Class>& x) const {
        return operator()(x.get());
    }
}; // struct const_member_base

template<class Class,typename Type,Type Class::*PtrToMember>
struct non_const_member_base {
    typedef Type result_type;

    template<typename ChainedPtr>
    auto operator()(const ChainedPtr& x) const
    -> std::enable_if_t<!std::is_convertible<const ChainedPtr&, const Class&>::type, Type&> {
        return operator()(*x);
    }

    const Type& operator()(const Class& x) const {
        return x.*PtrToMember;
    }
    Type& operator()(Class& x) const {
        return x.*PtrToMember;
    }
    const Type& operator()(const std::reference_wrapper<const Class>& x) const {
        return operator()(x.get());
    }
    Type& operator()(const std::reference_wrapper<Class>& x) const {
        return operator()(x.get());
    }
}; // struct non_const_member_base
}

template<class Class,typename Type,Type Class::*PtrToMember>
struct member: std::conditional<
    std::is_const<Type>::value,
    _detail::const_member_base<Class,Type,PtrToMember>,
    _detail::non_const_member_base<Class,Type,PtrToMember>
  >::type
{
};

//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//
template<class T> class intrusive_ptr
{
private:

    typedef intrusive_ptr this_type;

public:

    typedef T element_type;

    intrusive_ptr() BOOST_NOEXCEPT : px( 0 ) {
    }

    intrusive_ptr( T * p, bool add_ref = true ): px( p ) {
        if( px != 0 && add_ref ) intrusive_ptr_add_ref( px );
    }

    intrusive_ptr(intrusive_ptr const & rhs): px( rhs.px ) {
        if( px != 0 ) intrusive_ptr_add_ref( px );
    }

    ~intrusive_ptr() {
        if( px != 0 ) intrusive_ptr_release( px );
    }

    intrusive_ptr & operator=(intrusive_ptr const & rhs) {
        this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr & operator=(T * rhs) {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset() BOOST_NOEXCEPT {
        this_type().swap( *this );
    }

    void reset( T * rhs ) {
        this_type( rhs ).swap( *this );
    }

    void reset( T * rhs, bool add_ref ) {
        this_type( rhs, add_ref ).swap( *this );
    }

    T * get() const BOOST_NOEXCEPT {
        return px;
    }

    T * detach() BOOST_NOEXCEPT {
        T * ret = px;
        px = 0;
        return ret;
    }

    operator bool () const BOOST_NOEXCEPT {
        return px != 0;
    }

    T & operator*() const {
        BOOST_ASSERT( px != 0 );
        return *px;
    }

    T * operator->() const {
        BOOST_ASSERT( px != 0 );
        return px;
    }

    void swap(intrusive_ptr & rhs) BOOST_NOEXCEPT {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

private:
    T * px;
};

namespace _detail {
    template <typename T> constexpr T& min(T& a, T& b) {
        return a > b ? b : a;
    }

    template<int I, int Max> struct comparator_helper {
        template<typename L, typename V>
        bool operator()(const L& left, const V& value) const {
            return std::get<I>(left) == std::get<I>(value) &&
                comparator_helper<I + 1, Max>()(left, value);
        }
    }; // struct comparator_helper

    template<int I> struct comparator_helper<I, I> {
        template<typename... L> bool operator()(L&&...) const { return true; }
    }; // struct comparator_helper

    template<int I, int Max> struct converter_helper {
        template<typename L, typename V> void operator()(L& left, const V& value) const {
            std::get<I>(left) = std::get<I>(value);
            converter_helper<I + 1, Max>()(left, value);
        }
    }; // struct converter_helper

    template<int I> struct converter_helper<I, I> {
        template<typename... L> void operator()(L&&...) const { }
    }; // struct converter_helper
} // namespace _detail

template<typename Key>
struct key_comparator {
    static bool compare_eq(const Key& left, const Key& right) {
        return left == right;
    }
}; // struct key_comparator

template<typename... Indices>
struct key_comparator<std::tuple<Indices...>> {
    using key_type = std::tuple<Indices...>;

    template<typename Value>
    static bool compare_eq(const key_type& left, const Value& value) {
        return std::get<0>(left) == value;
    }

    template<typename... Values>
    static bool compare_eq(const key_type& left, const std::tuple<Values...>& value) {
        using value_type = std::tuple<Values...>;
        using namespace _detail;

        return comparator_helper<0, min(std::tuple_size<value_type>::value, std::tuple_size<key_type>::value)>()(left, value);
    }
}; // struct key_comparator

template<typename Key>
struct key_converter {
    static Key convert(const Key& key) {
        return key;
    }
}; // struct key_converter

template<typename... Indices>
struct key_converter<std::tuple<Indices...>> {
    using key_type = std::tuple<Indices...>;

    template<typename Value>
    static key_type convert(const Value& value) {
        key_type index;
        std::get<0>(index) = value;
        return index;
    }

    template<typename... Values>
    static key_type convert(const std::tuple<Values...>& value) {
        using namespace _detail;
        using value_type = std::tuple<Values...>;

        key_type index;
        converter_helper<0, min(std::tuple_size<value_type>::value, std::tuple_size<key_type>::value)>()(index, value);
        return index;
    }
}; // struct key_converter

struct service_info {
    account_name_t payer;
    int  size   = 0;
    bool in_ram = false;
}; // struct service_info

template<typename T, typename MultiIndex>
struct multi_index_item: public T {
    template<typename Constructor>
    multi_index_item(const MultiIndex& midx, Constructor&& constructor)
    : code_(midx.get_code()),
      scope_(midx.get_scope()) {
        constructor(*this);
    }

    const account_name_t code_;
    const scope_t scope_ = 0;
    service_info  service_;

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

template<index_name_t IndexName, typename Extractor>
struct indexed_by {
    enum constants { index_name = static_cast<uint64_t>(IndexName) };
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

template<table_name_t TableName, index_name_t IndexName> struct lower_bound final {
    template<typename Key>
    cursor_t operator()(account_name_t code, scope_t scope, const Key& key) const {
        cursor_t cursor;
        safe_allocate(pack_size(key), "Invalid size of key on lower_bound", [&](auto& data, auto& size) {
            pack_object(key, data, size);
            cursor = chaindb_lower_bound(code, scope, TableName, IndexName, data, size);
        });
        return cursor;
    }
}; // struct lower_bound

template<table_name_t TableName> struct lower_bound<TableName, N(primary)> final {
    cursor_t operator()(account_name_t code, scope_t scope, const primary_key_t pk) const {
        return chaindb_lower_bound_pk(code, scope, TableName, pk);
    }
}; // struct lower_bound

template<table_name_t TableName, index_name_t IndexName> struct upper_bound final {
    template<typename Key>
    cursor_t operator()(account_name_t code, scope_t scope, const Key& key) const {
        cursor_t cursor;
        safe_allocate(pack_size(key), "Invalid size of key on upper_bound", [&](auto& data, auto& size) {
            pack_object(key, data, size);
            cursor = chaindb_upper_bound(code, scope, TableName, IndexName, data, size);
        });
        return cursor;
    }
}; // struct upper_bound

template<table_name_t TableName> struct upper_bound<TableName, N(primary)> final {
    cursor_t operator()(account_name_t code, scope_t scope, const primary_key_t pk) const {
        return chaindb_upper_bound_pk(code, scope, TableName, pk);
    }
}; // struct upper_bound

template<uint64_t TableName, typename T, typename... Indices>
class multi_index {
private:
    static_assert(sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices");

    constexpr static bool validate_table_name(table_name_t n) {
        // Limit table names to 12 characters so that
        // the last character (4 bits) can be used to distinguish between the secondary indices.
        return (static_cast<uint64_t>(n) & 0x000000000000000FULL) == 0;
    }

    static_assert(
        validate_table_name(TableName),
        "multi_index does not support table names with a length greater than 12");

    using item = multi_index_item<T, multi_index>;
    using item_ptr = intrusive_ptr<item>;
    using primary_key_extractor_type = primary_key_extractor;

    template<index_name_t IndexName, typename Extractor>
    struct iterator_extractor_impl {
        template<typename Iterator>
        auto operator()(const Iterator& itr) const {
            return Extractor()(*itr);
        }
    }; // iterator_extractor_impl

    template<typename E>
    struct iterator_extractor_impl<N(primary), E> {
        template<typename Iterator>
        const primary_key_t& operator()(const Iterator& itr) const {
            return itr.primary_key_;
        }
    }; // iterator_extractor_impl

    template<index_name_t IndexName, typename Extractor> struct index;

    const account_name_t code_;
    const scope_t scope_;

    mutable primary_key_t next_primary_key_ = end_primary_key;

    using cache_vector_t_ = std::vector<item_ptr>;
    struct cache_item_t_ {
        const account_name_t code;
        const scope_t scope;
        cache_vector_t_ items_vector;

        cache_item_t_(const account_name_t c, const scope_t s)
        : code(c), scope(s) {
        }
    };

// Singleton realization for multiple instances
//    static cache_vector_t_& get_items_vector(const account_name_t code, const scope_t scope) {
//        static std::list<cache_item_t_> cache_map;
//        for (auto& itm: cache_map) {
//            if (itm.code == code && itm.scope == scope) return itm.items_vector;
//        }
//        cache_map.push_back({code, scope});
//        return cache_map.back().items_vector;
//    }
//
//    cache_vector_t_& items_vector_;
    mutable cache_vector_t_ items_vector_;

    template<index_name_t IndexName>
    struct const_iterator_impl: public std::iterator<std::bidirectional_iterator_tag, const T> {
    public:
        friend bool operator == (const const_iterator_impl& a, const const_iterator_impl& b) {
            a.lazy_open();
            b.lazy_open();
            return a.primary_key_ == b.primary_key_;
        }
        friend bool operator != (const const_iterator_impl& a, const const_iterator_impl& b) {
            return !(operator == (a, b));
        }

        constexpr static table_name_t table_name() { return TableName; }
        constexpr static index_name_t index_name() { return IndexName; }
        account_name_t get_code() const            { return multidx_->get_code(); }
        scope_t        get_scope() const           { return multidx_->get_scope(); }

        const T& operator*() const {
            lazy_load_object();
            return *static_cast<const T*>(item_.get());
        }
        const T* operator->() const {
            lazy_load_object();
            return static_cast<const T*>(item_.get());
        }
        primary_key_t pk() const {
            lazy_load_object();
            return primary_key_;
        }
        int size() const {
            lazy_load_object();
            return item_->service_.size;
        }
        eosio::name payer() const {
            lazy_load_object();
            return item_->service_.payer;
        }

        const_iterator_impl operator++(int) {
            const_iterator_impl result(*this);
            ++(*this);
            return result;
        }
        const_iterator_impl& operator++() {
            lazy_open();
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
            lazy_open();
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

            src.cursor_ = uninitialized_state;
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

            if (src.is_cursor_initialized()) {
                cursor_ = chaindb_clone(get_code(), src.cursor_);
            } else {
                cursor_ = src.cursor_;
            }

            primary_key_ = src.primary_key_;
            item_ = src.item_;

            return *this;
        }

        ~const_iterator_impl() {
            if (is_cursor_initialized()) chaindb_close(get_code(), cursor_);
        }

    private:
        friend multi_index;
        template<index_name_t, typename> friend struct iterator_extractor_impl;

        const_iterator_impl(const multi_index* midx, const cursor_t cursor)
        : multidx_(midx), cursor_(cursor) {
            if (is_cursor_initialized()) {
                primary_key_ = chaindb_current(get_code(), cursor_);
            }
        }

        const_iterator_impl(const multi_index* midx, const cursor_t cursor, const primary_key_t pk, item_ptr item)
        : multidx_(midx), cursor_(cursor), primary_key_(pk), item_(item) { }

        const multi_index* multidx_ = nullptr;
        mutable cursor_t cursor_ = uninitialized_state;
        mutable primary_key_t primary_key_ = end_primary_key;
        mutable item_ptr item_;

        void lazy_load_object() const {
            if (item_ && !item_->deleted_) return;

            lazy_open();
            chaindb_assert(primary_key_ != end_primary_key, "cannot load object from end iterator");
            item_ = multidx_->load_object(cursor_, primary_key_);
        }

        void lazy_open() const {
            if (BOOST_LIKELY(is_cursor_initialized())) return;

            switch (cursor_) {
                case uninitialized_begin:
                    lazy_open_begin();
                    break;

                case uninitialized_end:
                    lazy_open_end();
                    break;

                case uninitialized_find_by_pk:
                    lazy_open_find_by_pk();
                    break;

                default:
                    break;
            }
            chaindb_assert(is_cursor_initialized(), "unable to open cursor");
        }

        bool is_cursor_initialized() const {
            return cursor_ > uninitialized_state;
        }

        void lazy_open_begin() const {
            cursor_ = chaindb_begin(get_code(), get_scope(), table_name(), index_name());
            chaindb_assert(is_cursor_initialized(), "unable to open begin iterator");
            primary_key_ = chaindb_current(get_code(), cursor_);
        }

        void lazy_open_end() const {
            cursor_ = chaindb_end(get_code(), get_scope(), table_name(), index_name());
            chaindb_assert(is_cursor_initialized(), "unable to open end iterator");
        }

        void lazy_open_find_by_pk() const {
            cursor_ = chaindb_lower_bound_pk(get_code(), get_scope(), table_name(), primary_key_);
            chaindb_assert(is_cursor_initialized(), "unable to open find_by_pk iterator");

            auto pk = chaindb_current(get_code(), cursor_);
            chaindb_assert(primary_key_ == pk, "primary key from cursor does not equal expected");
        }

    private:
        static constexpr cursor_t uninitialized_state = 0;
        static constexpr cursor_t uninitialized_end = -1;
        static constexpr cursor_t uninitialized_begin = -2;
        static constexpr cursor_t uninitialized_find_by_pk = -3;
    }; /// struct multi_index::const_iterator_impl

    template<index_name_t IndexName, typename Extractor>
    struct index {
    public:
        using extractor_type = Extractor;
        using iterator_extractor_type = iterator_extractor_impl<IndexName, Extractor>;
        using key_type = typename std::decay<decltype(Extractor()(static_cast<const T&>(*(const T*)nullptr)))>::type;
        using const_iterator = const_iterator_impl<IndexName>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr static bool validate_index_name(index_name_t n) {
           return static_cast<uint64_t>(n) != 0;
        }

        static_assert(validate_index_name(IndexName), "invalid index name used in multi_index");

        constexpr static table_name_t table_name() { return TableName; }
        constexpr static index_name_t index_name() { return IndexName; }
        account_name_t get_code() const            { return multidx_->get_code(); }
        scope_t        get_scope() const           { return multidx_->get_scope(); }

        const_iterator cbegin() const {
            return {multidx_, const_iterator::uninitialized_begin};
        }
        const_iterator begin() const  { return cbegin(); }

        const_iterator cend() const {
            return {multidx_, const_iterator::uninitialized_end};
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
            if (!key_comparator<key_type>::compare_eq(iterator_extractor_type()(itr), value)) return etr;
            return itr;
        }
        const_iterator find(const key_type& key) const {
           auto itr = lower_bound(key);
           auto etr = cend();
           if (itr == etr) return etr;
           if (key != iterator_extractor_type()(itr)) return etr;
           return itr;
        }

        const_iterator require_find(key_type&& key, const char* error_msg = "unable to find key") const {
            return require_find(key, error_msg);
        }
        const_iterator require_find(const key_type& key, const char* error_msg = "unable to find key") const {
            auto itr = lower_bound(key);
            chaindb_assert(itr != cend(), error_msg);
            chaindb_assert(key == iterator_extractor_type()(itr), error_msg);
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
            eosio::lower_bound<TableName, IndexName> finder;
            auto cursor = finder(get_code(), get_scope(), key);
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
            eosio::upper_bound<TableName, IndexName> finder;
            auto cursor = finder(get_code(), get_scope(), key);
            return const_iterator(multidx_, cursor);
        }

        const_iterator iterator_to(const T& obj) const {
            const auto& itm = static_cast<const item&>(obj);
            chaindb_assert(multidx_->is_same_multidx(itm), "object passed to iterator_to is not in multi_index");

            auto key = extractor_type()(itm);
            auto pk = primary_key_extractor_type()(itm);
            cursor_t cursor;
            safe_allocate(pack_size(key), "invalid size of key", [&](auto& data, auto& size) {
                pack_object(key, data, size);
                cursor = chaindb_locate_to(get_code(), get_scope(), table_name(), index_name(), pk, data, size);
            });

            return const_iterator(multidx_, cursor, pk, item_ptr(const_cast<item*>(&itm), false));
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

        safe_allocate(size, "object doesn't exist", [&](auto& data, auto& datasize) {
            auto dpk = chaindb_data(get_code(), cursor, data, datasize);
            chaindb_assert(dpk == pk, "invalid packet object");
            ptr = item_ptr(new item(*this, [&](auto& itm) {
                T& obj = static_cast<T&>(itm);
                unpack_object(obj, data, datasize);
            }));
        });

        auto ptr_pk = primary_key_extractor_type()(*ptr);
        chaindb_assert(ptr_pk == pk, "invalid primary key of object");

        safe_allocate(sizeof(service_info), "object doesn't exist", [&](auto& data, auto& datasize) {
            chaindb_service(get_code(), cursor, data, datasize);
            unpack_object(ptr->service_, data, datasize);
        });

        add_object_to_cache(ptr);
        return ptr;
    }

    bool is_same_multidx(const item& o) const {
        return (o.code_ == get_code() && o.scope_ == get_scope());
    }

public:
    using const_iterator = const_iterator_impl<N(primary)>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    multi_index(const account_name_t code, const scope_t scope)
    : code_(code), scope_(scope), primary_idx_(this) {
    }

    constexpr static table_name_t table_name() { return TableName; }

    account_name_t get_code() const  { return code_; }
    scope_t get_scope() const { return scope_; }

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
                bool, std::decay<decltype(in)>::type::index_name == static_cast<uint64_t>(IndexName)>();
        });

        static_assert(res != hana::nothing, "name provided is not the name of any secondary index within multi_index");

        return index<IndexName, typename std::decay<decltype(res.value())>::type::extractor_type>(this);
    }

    const_iterator iterator_to(const T& obj) const {
        return primary_idx_.iterator_to(obj);
    }

    void flush_cache() {
        for (auto& itm_ptr: items_vector_) {
            itm_ptr->deleted_ = true;
        }
        items_vector_.clear();
    }

    template<typename Lambda>
    const_iterator emplace(const account_name_t payer, Lambda&& constructor) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
        CHAINDB_ANOTHER_CONTRACT_PROTECT(
            static_cast<uint64_t>(get_code()) == current_receiver(),
            "cannot create objects in table of another contract");

        auto ptr = item_ptr(new item(*this, [&](auto& itm) {
            constructor(static_cast<T&>(itm));
        }));

        auto& obj = static_cast<T&>(*ptr);
        auto pk = primary_key_extractor_type()(obj);
        chaindb_assert(pk != end_primary_key, "invalid value of primary key");

        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            auto delta = chaindb_insert(get_code(), get_scope(), table_name(), payer, pk, data, size);
            ptr->service_.size   = delta;
            ptr->service_.in_ram = true;
            ptr->service_.payer  = payer;
        });

        add_object_to_cache(ptr);

        next_primary_key_ = pk + 1;
        return const_iterator(this, const_iterator::uninitialized_find_by_pk, pk, std::move(ptr));
    }

    template<typename Lambda>
    void modify(const_iterator itr, const account_name_t payer, Lambda&& updater) const {
        chaindb_assert(itr != end(), "cannot pass end iterator to modify");
        modify(*itr, payer, std::forward<Lambda&&>(updater));
    }

    template<typename Lambda>
    void modify(const T& obj, const account_name_t payer, Lambda&& updater) const {
        // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.
        CHAINDB_ANOTHER_CONTRACT_PROTECT(
            static_cast<uint64_t>(get_code()) == current_receiver(),
            "cannot modify objects in table of another contract");

        auto& mobj = const_cast<T&>(obj);
        auto& itm = static_cast<item&>(mobj);
        chaindb_assert(is_same_multidx(itm), "object passed to modify is not in multi_index");
        chaindb_assert(itm.service_.in_ram, "object passed to modify is in archive");

        auto pk = primary_key_extractor_type()(obj);

        updater(mobj);

        auto mpk = primary_key_extractor_type()(obj);
        chaindb_assert(pk == mpk, "updater cannot change primary key when modifying an object");

        safe_allocate(pack_size(obj), "invalid size of object", [&](auto& data, auto& size) {
            pack_object(obj, data, size);
            auto delta = chaindb_update(get_code(), get_scope(), table_name(), payer, pk, data, size);
            itm.service_.payer = payer;
            itm.service_.size += delta;
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

        CHAINDB_ANOTHER_CONTRACT_PROTECT(
            static_cast<uint64_t>(get_code()) == current_receiver(),
            "cannot delete objects from table of another contract");

        chaindb_assert(is_same_multidx(itm), "object passed to erase is not in multi_index");

        auto pk = primary_key_extractor_type()(obj);
        remove_object_from_cache(pk);
        chaindb_delete(get_code(), get_scope(), table_name(), pk);
    }

    void move_to_ram(const T& obj) const {
        auto& itm = static_cast<item&>(const_cast<T&>(obj));

        CHAINDB_ANOTHER_CONTRACT_PROTECT(
            static_cast<uint64_t>(get_code()) == current_receiver(),
            "cannot move objects from table of another contract");

        chaindb_assert(is_same_multidx(itm), "object passed to move_to_ram is not in multi_index");
        chaindb_assert(!itm.service_.in_ram, "object passed to move_to_ram is already in RAM");
        auto pk = primary_key_extractor_type()(obj);
        chaindb_ram_state(get_code(), get_scope(), table_name(), pk, true);
        itm.service_.in_ram = true;
    }

    void move_to_archive(const T& obj) const {
        auto& itm = static_cast<item&>(const_cast<T&>(obj));

        CHAINDB_ANOTHER_CONTRACT_PROTECT(
            static_cast<uint64_t>(get_code()) == current_receiver(),
            "cannot move objects from table of another contract");

        chaindb_assert(is_same_multidx(itm), "object passed to move_to_archive is not in multi_index");
        chaindb_assert(itm.service_.in_ram,  "object passed to move_to_archive is already in archive");
        auto pk = primary_key_extractor_type()(obj);
        chaindb_ram_state(get_code(), get_scope(), table_name(), pk, false);
        itm.service_.in_ram = false;
    }

}; // class multi_index
}  // namespace eosio
