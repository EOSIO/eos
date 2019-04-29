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

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/storage_payer_info.hpp>

namespace boost { namespace tuples {

namespace _detail {
    template<int I, int IMax> struct pack {
        template<typename T, typename V> void operator()(T& stream, const V& value) const {
            fc::raw::pack(stream, get<I>(value));
            pack<I + 1, IMax>()(stream, value);
        }
    }; // struct pack

    template<int I> struct pack<I, I> {
        template<typename... T> void operator()(T&&...) const { }
    }; // struct pack
} // namespace _detail

template<typename T, typename... Indicies>
fc::datastream<T>& operator<<(fc::datastream<T>& stream, const boost::tuple<Indicies...>& value) {
    _detail::pack<0, length<tuple<Indicies...>>::value>()(stream, value);
    return stream;
}

} } // namespace boost::tuples

namespace cyberway { namespace chaindb {
    template<class> struct tag;
    template<class> struct object_to_index;
} }

namespace cyberway { namespace chaindb {
    namespace uninitilized_cursor {
        static constexpr cursor_t state = 0;
        static constexpr cursor_t end = -1;
        static constexpr cursor_t begin = -2;
    }

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

template<typename Value, typename Lambda>
void safe_allocate(const Value& value, const char* error_msg, Lambda&& callback) {
    auto size = fc::raw::pack_size(value);
    CYBERWAY_ASSERT(size > 0, chaindb_midx_logic_exception, error_msg);

    constexpr static size_t max_stack_data_size = 65536;

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
    CYBERWAY_ASSERT(alloc.data != nullptr, chaindb_midx_logic_exception, "Unable to allocate memory");

    callback(alloc.data, alloc.size);
}

template<bool IsPrimaryIndex> struct lower_bound final {
    template<typename Key>
    find_info operator()(chaindb_controller& chaindb, const index_request& request, const Key& key) const {
        find_info info;
        safe_allocate(key, "Invalid size of key on lower_bound", [&](auto& data, auto& size) {
            pack_object(key, data, size);
            info = chaindb.lower_bound(request, data, size);
        });
        return info;
    }
}; // struct lower_bound

template<> struct lower_bound<true /*IsPrimaryIndex*/> final {
    find_info operator()(chaindb_controller& chaindb, const table_request& request, const primary_key_t pk) const {
        return chaindb.lower_bound(request, pk);
    }
}; // struct lower_bound

template<bool IsPrimaryIndex> struct upper_bound final {
    template<typename Key>
    find_info operator()(chaindb_controller& chaindb, const index_request& request, const Key& key) const {
        find_info info;
        safe_allocate(key, "Invalid size of key on upper_bound", [&](auto& data, auto& size) {
            pack_object(key, data, size);
            info = chaindb.upper_bound(request, data, size);
        });
        return info;
    }
}; // struct upper_bound

template<> struct upper_bound<true /*IsPrimaryIndex*/> final {
    find_info operator()(chaindb_controller& chaindb, const table_request& request, const primary_key_t pk) const {
        return chaindb.upper_bound(request, pk);
    }
}; // struct upper_bound

using boost::multi_index::const_mem_fun;

namespace _multi_detail {
    template <typename T> constexpr T& min(T& a, T& b) {
        return a > b ? b : a;
    }

    template<int I, int Max> struct comparator_helper {
        template<typename L, typename V>
        bool operator()(const L& left, const V& value) const {
            return boost::tuples::get<I>(left) == boost::tuples::get<I>(value) &&
                comparator_helper<I + 1, Max>()(left, value);
        }
    }; // struct comparator_helper

    template<int I> struct comparator_helper<I, I> {
        template<typename... L> bool operator()(L&&...) const { return true; }
    }; // struct comparator_helper

    template<int I, int Max> struct converter_helper {
        template<typename L, typename V> void operator()(L& left, const V& value) const {
            boost::tuples::get<I>(left) = boost::tuples::get<I>(value);
            converter_helper<I + 1, Max>()(left, value);
        }
    }; // struct converter_helper

    template<int I> struct converter_helper<I, I> {
        template<typename... L> void operator()(L&&...) const { }
    }; // struct converter_helper
} // namespace _multi_detail

template<typename Key>
struct key_comparator {
    static bool compare_eq(const Key& left, const Key& right) {
        return left == right;
    }
}; // struct key_comparator

template<typename... Indices>
struct key_comparator<boost::tuple<Indices...>> {
    using key_type = boost::tuple<Indices...>;

    template<typename Value>
    static bool compare_eq(const key_type& left, const Value& value) {
        return boost::tuples::get<0>(left) == value;
    }

    template<typename... Values>
    static bool compare_eq(const key_type& left, const boost::tuple<Values...>& value) {
        using value_type = boost::tuple<Values...>;
        using namespace _multi_detail;
        using boost::tuples::length;

        return comparator_helper<0, min(length<value_type>::value, length<key_type>::value)>()(left, value);
    }
}; // struct key_comparator

template<typename Key>
struct key_converter {
    static const Key& convert(const Key& key) {
        return key;
    }
}; // struct key_converter

template<typename... Indices>
struct key_converter<boost::tuple<Indices...>> {
    using key_type = boost::tuple<Indices...>;

    template<typename Value>
    static key_type convert(const Value& value) {
        key_type index;
        boost::tuples::get<0>(index) = value;
        return index;
    }

    template<typename... Values>
    static key_type convert(const boost::tuple<Values...>& value) {
        using namespace _multi_detail;
        using boost::tuples::length;
        using value_type = boost::tuple<Values...>;

        key_type index;
        converter_helper<0, min(length<value_type>::value, length<key_type>::value)>()(index, value);
        return index;
    }
}; // struct key_converter

template<typename C> struct code_extractor;

template<typename C> struct code_extractor<tag<C>> {
    constexpr static uint64_t get_code() {
        return tag<C>::get_code();
    }
}; // struct code_extractor

template<typename C> struct code_extractor: public code_extractor<tag<C>> {
    constexpr static uint64_t get_code() {
        return tag<C>::get_code();
    }
}; // struct code_extractor

template <typename T>
struct multi_index_item_data final: public cache_data {
    struct item_type: public T {
        template<typename Constructor>
        item_type(cache_object& cache, Constructor&& constructor)
            : T(std::forward<Constructor>(constructor), 0),
              cache(cache) {
        }

        cache_object& cache;
    } item;

    template<typename Constructor>
    multi_index_item_data(cache_object& cache, Constructor&& constructor)
    : item(cache, std::forward<Constructor>(constructor)) {
    }

    static const T& get_T(const cache_object& cache) {
        return static_cast<const T&>(cache.data<const multi_index_item_data>().item);
    }

    static const T& get_T(const cache_object_ptr& cache) {
        return get_T(*cache.get());
    }

    static cache_object& get_cache(const T& o) {
        return const_cast<cache_object&>(static_cast<const item_type&>(o).cache);
    }

    using value_type = T;
}; // struct multi_index_item_data

template<typename TableName, typename PrimaryIndex, typename T, typename... Indices>
struct multi_index final {
public:
    template<typename IndexName, typename Extractor> class index;

private:
    static_assert(sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices");

    using item_data = multi_index_item_data<T>;

    struct cache_converter_ final: public cache_converter_interface {
        cache_converter_()  = default;
        ~cache_converter_() = default;

        void convert_variant(cache_object& cache, const object_value& obj) const override {
            cache.set_data<item_data>([&](auto& o) {
                T& data = static_cast<T&>(o);
                fc::from_variant(obj.value, data);
            });

            const auto ptr_pk = primary_key_extractor_type()(item_data::get_T(cache));
            CYBERWAY_ASSERT(ptr_pk == obj.pk(), chaindb_midx_pk_exception,
                "invalid primary key ${pk} for the object ${obj}", ("pk", obj.pk())("obj", obj.value));
        }
    }; // struct cache_converter_

    using primary_key_extractor_type = typename PrimaryIndex::extractor;

    template<typename IndexName, typename Extractor>
    struct iterator_extractor_impl {
        template<typename Iterator>
        auto operator()(const Iterator& itr) const {
            return Extractor()(*itr);
        }
    }; // iterator_extractor_impl

    template<typename E>
    struct iterator_extractor_impl<typename PrimaryIndex::tag, E> {
        template<typename Iterator>
        const primary_key_t& operator()(const Iterator& itr) const {
            return itr.primary_key_;
        }
    }; // iterator_extractor_impl

    static string get_index_name(const index_name_t index) {
        return eosio::chain::table_name(table_name()).to_string()
            .append(".")
            .append(eosio::chain::index_name(index).to_string());
    }

    static string get_index_name() {
        return get_index_name(primary_index_type::index_name());
    }

    template<typename IndexName>
    class const_iterator_impl final: public std::iterator<std::bidirectional_iterator_tag, const T> {
    public:
        friend bool operator == (const const_iterator_impl& a, const const_iterator_impl& b) {
            a.lazy_open();
            b.lazy_open();
            return a.primary_key_ == b.primary_key_;
        }
        friend bool operator != (const const_iterator_impl& a, const const_iterator_impl& b) {
            return !(operator == (a, b));
        }

        constexpr static table_name_t   table_name() { return code_extractor<TableName>::get_code(); }
        constexpr static index_name_t   index_name() { return code_extractor<IndexName>::get_code(); }
        constexpr static account_name_t code_name()  { return 0; }
        constexpr static account_name_t scope_name() { return 0; }

        const T& operator*() const {
            lazy_load_object();
            return item_data::get_T(item_);
        }
        const T* operator->() const {
            lazy_load_object();
            return &item_data::get_T(item_);
        }
        const primary_key_t pk() const {
            lazy_open();
            return primary_key_;
        }
        const service_state& service() const {
            lazy_load_object();
            return item_->service();
        }
        bool in_ram() const {
            return service().in_ram;
        }

        const_iterator_impl operator++(int) {
            const_iterator_impl result(*this);
            ++(*this);
            return result;
        }
        const_iterator_impl& operator++() {
            lazy_open();
            CYBERWAY_ASSERT(primary_key_ != end_primary_key, chaindb_midx_pk_exception,
                "Can't increment end iterator for the index ${index}", ("index", get_index_name()));
            primary_key_ = controller().next(get_cursor_request());
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
            primary_key_ = controller().prev(get_cursor_request());
            item_.reset();
            CYBERWAY_ASSERT(primary_key_ != end_primary_key, chaindb_midx_pk_exception,
                "Out of range on decrement of iterator for the index ${index}", ("index", get_index_name()));
            return *this;
        }

        const_iterator_impl() = default;

        const_iterator_impl(const_iterator_impl&& src) {
            this->operator=(std::move(src));
        }
        const_iterator_impl& operator=(const_iterator_impl&& src) {
            if (this == &src) return *this;

            controller_ = src.controller_;
            cursor_ = src.cursor_;
            primary_key_ = src.primary_key_;
            item_ = std::move(src.item_);

            src.cursor_ = uninitilized_cursor::state;
            src.primary_key_ = end_primary_key;

            return *this;
        }

        const_iterator_impl(const const_iterator_impl& src) {
            this->operator=(src);
        }
        const_iterator_impl& operator=(const const_iterator_impl& src) {
            if (this == &src) return *this;

            controller_ = src.controller_;
            primary_key_ = src.primary_key_;
            item_ = src.item_;

            if (src.is_cursor_initialized()) {
                cursor_ = controller().clone(src.get_cursor_request()).cursor;
            } else {
                cursor_ = src.cursor_;
            }

            return *this;
        }

        ~const_iterator_impl() {
            if (is_cursor_initialized()) controller().close(get_cursor_request());
        }

    private:
        bool is_cursor_initialized() const {
            return cursor_ > uninitilized_cursor::state;
        }

        template<typename, typename> friend class index;
        template<typename, typename> friend struct iterator_extractor_impl;

        const_iterator_impl(chaindb_controller* ctrl, const cursor_t cursor)
        : controller_(ctrl), cursor_(cursor) { }

        const_iterator_impl(chaindb_controller* ctrl, const cursor_t cursor, const primary_key_t pk)
        : controller_(ctrl), cursor_(cursor), primary_key_(pk) { }

        const_iterator_impl(chaindb_controller* ctrl, const cursor_t cursor, const primary_key_t pk, cache_object_ptr item)
        : controller_(ctrl), cursor_(cursor), primary_key_(pk), item_(std::move(item)) { }

        chaindb_controller& controller() const {
            CYBERWAY_ASSERT(controller_, chaindb_midx_logic_exception,
                "Iterator is not initialized for the index ${index}", ("index", get_index_name()));
            return *controller_;
        }

        chaindb_controller* controller_ = nullptr;
        mutable cursor_t cursor_ = uninitilized_cursor::state;
        mutable primary_key_t primary_key_ = end_primary_key;
        mutable cache_object_ptr item_;

        void lazy_load_object() const {
            if (item_ && !item_->is_deleted()) return;

            lazy_open();
            CYBERWAY_ASSERT(primary_key_ != end_primary_key, chaindb_midx_pk_exception,
                "Cannot load object from the end iterator for the index ${index}", ("index", get_index_name()));

            auto ptr = controller().get_cache_object({code_name(), cursor_}, false);

            auto& obj = item_data::get_T(ptr);
            auto ptr_pk = primary_key_extractor_type()(obj);
            CYBERWAY_ASSERT(ptr_pk == primary_key_, chaindb_midx_pk_exception,
                "Invalid primary key ${pk} of object ${obj} for the index ${index}",
                ("pk", primary_key_)("obj", obj)("index", get_index_name()));
            item_ = ptr;
        }

        void lazy_open_begin() const {
           if (BOOST_LIKELY(uninitilized_cursor::begin != cursor_)) return;

            auto info = controller().begin(get_index_request());
            cursor_ = info.cursor;
            primary_key_ = info.pk;
        }

        void lazy_open() const {
            if (BOOST_LIKELY(is_cursor_initialized())) return;

            switch (cursor_) {
                case uninitilized_cursor::begin:
                    lazy_open_begin();
                    break;

                case uninitilized_cursor::end:
                    cursor_ = controller().end(get_index_request()).cursor;
                    break;

                default:
                    break;
            }
            CYBERWAY_ASSERT(is_cursor_initialized(), chaindb_midx_pk_exception,
                "Unable to open cursor for the pk ${pk} for the index ${index}",
                ("pk", primary_key_)("index", get_index_name()));
        }

        static const table_request& get_table_request() {
            return multi_index::get_table_request();
        }

        static const index_request& get_index_request() {
            static index_request request{code_name(), scope_name(), table_name(), index_name()};
            return request;
        }

        static string get_index_name() {
            return multi_index::get_index_name(index_name());
        }

        cursor_request get_cursor_request() const {
            return {code_name(), cursor_};
        }
    }; // struct multi_index::const_iterator_impl

    template<typename... Idx>
    struct index_container {
        using type = decltype(boost::hana::tuple<Idx...>());
    };

    using indices_type = typename index_container<Indices...>::type;

    indices_type indices_;

    using primary_index_type = index<typename PrimaryIndex::tag, typename PrimaryIndex::extractor>;
    primary_index_type primary_idx_;

public:
    using const_iterator = typename primary_index_type::const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using emplace_result = typename primary_index_type::emplace_result;

    template<typename IndexName, typename Extractor>
    class index {
    public:
        struct emplace_result final {
            const T& obj;
            int64_t  delta;
        }; // struct emplace_result

        using extractor_type = Extractor;
        using iterator_extractor_type = iterator_extractor_impl<IndexName, Extractor>;
        using key_type = typename std::decay<decltype(Extractor()(static_cast<const T&>(*(const T*)nullptr)))>::type;
        using const_iterator = const_iterator_impl<IndexName>;

        constexpr static table_name_t   table_name() { return code_extractor<TableName>::get_code(); }
        constexpr static index_name_t   index_name() { return code_extractor<IndexName>::get_code(); }
        constexpr static account_name_t code_name()  { return 0; }
        constexpr static account_name_t scope_name() { return 0; }

        index(chaindb_controller& ctrl)
        : controller_(ctrl) {
        }

        const_iterator cbegin() const {
            return const_iterator(&controller_, uninitilized_cursor::begin);
        }
        const_iterator begin() const  { return cbegin(); }

        const_iterator cend() const {
            return const_iterator(&controller_, uninitilized_cursor::end);
        }
        const_iterator end() const  { return cend(); }

        template<typename Value>
        const_iterator find(const Value& value) const {
            auto itr = lower_bound(value);
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

        const T& get(const key_type& key) const {
            auto itr = find(key);
            CYBERWAY_ASSERT(itr != cend(), chaindb_midx_find_exception,
                "Unable to find key in the index ${index}", ("index", get_index_name()));
            return *itr;
        }

        const_iterator lower_bound(const key_type& key) const {
            chaindb::lower_bound<std::is_same<IndexName, typename PrimaryIndex::tag>::value> finder;
            auto info = finder(controller_, get_index_request(), key);
            return const_iterator(&controller_, info.cursor, info.pk);
        }

        template<typename Value>
        const_iterator lower_bound(const Value& value) const {
            return lower_bound(key_converter<key_type>::convert(value));
        }
        const_iterator upper_bound(const key_type& key) const {
            chaindb::upper_bound<std::is_same<IndexName, typename PrimaryIndex::tag>::value> finder;
            auto info = finder(controller_, get_index_request(), key);
            return const_iterator(&controller_, info.cursor, info.pk);
        }

        std::pair<const_iterator,const_iterator> equal_range(const key_type& key) const {
            return make_pair(lower_bound(key), upper_bound(key));
        }

        template<typename Value>
        std::pair<const_iterator,const_iterator> equal_range(const Value& value) const {
            const_iterator lower = lower_bound(value);
            const_iterator upper = lower;
            auto etr = cend();
            while(upper != etr && key_comparator<key_type>::compare_eq(iterator_extractor_type()(upper), value)) {
                ++upper;
            }

            return make_pair(lower, upper);
        }

        size_t size() const {
            // Bad realization, normal for tests,
            //   should NOT!!!! be used in the production code.
            return std::distance(begin(), end());
        }

        template<typename Lambda>
        emplace_result emplace(const storage_payer_info& payer, Lambda&& constructor) const {
            auto cache = controller_.create_cache_object(get_table_request());
            try {
                auto pk = cache->pk();
                cache->template set_data<item_data>([&](auto& o) {
                    o.id = pk;
                    constructor(static_cast<T&>(o));
                });

                auto& obj = item_data::get_T(cache);
                auto obj_pk = primary_key_extractor_type()(obj);
                CYBERWAY_ASSERT(obj_pk == pk, chaindb_midx_pk_exception,
                    "Invalid value of primary key ${pk} for the object ${obj} on emplace for the index ${index}",
                    ("pk", pk)("obj", obj)("index", get_index_name()));

                fc::variant value;
                fc::to_variant(obj, value);
                auto delta = controller_.insert(*cache.get(), std::move(value), payer);

                return {obj, delta};
            } catch (...) {
                cache->mark_deleted();
                throw;
            }
        }

        template<typename Lambda>
        emplace_result emplace(Lambda&& constructor) const {
            return emplace({}, std::forward<Lambda>(constructor));
        }

        template<typename Lambda>
        int modify(const T& obj, const storage_payer_info& payer, Lambda&& updater) const {
            auto& itm = item_data::get_cache(obj);
            CYBERWAY_ASSERT(itm.is_valid_table(get_table_request()), chaindb_midx_logic_exception,
                "Object ${obj} passed to modify is not from the index ${index}",
                ("obj", obj)("index", get_index_name()));

            auto pk = primary_key_extractor_type()(obj);

            auto& mobj = const_cast<T&>(obj);
            updater(mobj);

            auto mpk = primary_key_extractor_type()(obj);
            CYBERWAY_ASSERT(pk == mpk, chaindb_midx_pk_exception,
                "Updater change primary key ${pk} on modifying of the object ${obj} for the index ${index}",
                ("pk", pk)("obj", obj)("index", get_index_name()));

            fc::variant value;
            fc::to_variant(obj, value);
            return controller_.update(itm, std::move(value), payer);
        }

        template<typename Lambda>
        int modify(const T& obj, Lambda&& updater) const {
            return modify(obj, {}, std::forward<Lambda>(updater));
        }

        int erase(const T& obj, const storage_payer_info& payer = {}) const {
            auto& itm = item_data::get_cache(obj);
            CYBERWAY_ASSERT(itm.is_valid_table(get_table_request()), chaindb_midx_logic_exception,
                "Object ${obj} passed to erase is not from the index ${index}",
                ("obj", obj)("index", get_index_name()));

            return controller_.remove(itm, payer);
        }

        static auto extract_key(const T& obj) { return extractor_type()(obj); }

    private:
        friend multi_index;

        static const index_request& get_index_request() {
            static index_request request{code_name(), scope_name(), table_name(), index_name()};
            return request;
        }

        static string get_index_name() {
            return multi_index::get_index_name(index_name());
        }

        chaindb_controller& controller_;
    }; // struct multi_index::index

public:
    multi_index(chaindb_controller& controller)
    : primary_idx_(controller) {
    }

    constexpr static table_name_t   table_name() { return code_extractor<TableName>::get_code(); }
    constexpr static account_name_t code_name()  { return 0; }
    constexpr static account_name_t scope_name() { return 0; }

    static const table_request& get_table_request() {
        static table_request request{code_name(), scope_name(), table_name()};
        return request;
    }

    static void set_cache_converter(chaindb_controller& controller) {
        static cache_converter_ converter;
        controller.set_cache_converter(get_table_request(), converter);
    }

    const_iterator cbegin() const { return primary_idx_.cbegin(); }
    const_iterator begin() const  { return cbegin(); }

    const_iterator cend() const  { return primary_idx_.cend(); }
    const_iterator end() const   { return cend(); }


    const_iterator lower_bound(primary_key_t pk) const {
        return primary_idx_.lower_bound(pk);
    }

    const_iterator upper_bound(primary_key_t pk) const {
        return primary_idx_.upper_bound(pk);
    }

    const T& get(const primary_key_t pk = 0) const {
        return primary_idx_.get(pk);
    }

    const_iterator find(const primary_key_t pk = 0) const {
        return primary_idx_.find(pk);
    }

    template<typename IndexTag>
    auto get_index() const {
        namespace hana = boost::hana;

        auto res = hana::find_if(indices_, [](auto&& in) {
            return std::is_same<typename std::decay<decltype(in)>::type::tag_type, tag<IndexTag>>();
        });

        static_assert(res != hana::nothing, "name provided is not the name of any secondary index within multi_index");

        return index<IndexTag, typename std::decay<decltype(res.value())>::type::extractor_type>(primary_idx_.controller_);
    }

    size_t size() const {
        return primary_idx_.size();
    }

    template<typename Lambda>
    emplace_result emplace(Lambda&& constructor) const {
        return primary_idx_.emplace(std::forward<Lambda>(constructor));
    }

    template<typename Lambda>
    emplace_result emplace(const storage_payer_info& payer, Lambda&& constructor) const {
        return primary_idx_.emplace(payer, std::forward<Lambda>(constructor));
    }

    template<typename Lambda>
    int64_t modify(const T& obj, Lambda&& updater) const {
        return primary_idx_.modify(obj, std::forward<Lambda>(updater));
    }

    template<typename Lambda>
    int64_t modify(const T& obj, const storage_payer_info& payer, Lambda&& updater) const {
        return primary_idx_.modify(obj, payer, std::forward<Lambda>(updater));
    }

    int64_t erase(const T& obj, const storage_payer_info& payer = {}) const {
        return primary_idx_.erase(obj, payer);
    }
}; // struct multi_index

} }  // namespace cyberway::chaindb
