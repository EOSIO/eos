// Copyright (c) 2017 Alexandr Poltavsky, Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// The Great Type Loophole (C++14)
// Initial implementation by Alexandr Poltavsky, http://alexpolt.github.io
//
// Description: 
//  The Great Type Loophole is a technique that allows to exchange type information with template
//  instantiations. Basically you can assign and read type information during compile time.
//  Here it is used to detect data members of a data type. I described it for the first time in
//  this blog post http://alexpolt.github.io/type-loophole.html .
//
// This technique exploits the http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2118
// CWG 2118. Stateful metaprogramming via friend injection
// Note: CWG agreed that such techniques should be ill-formed, although the mechanism for prohibiting them is as yet undetermined.

#ifndef BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP
#define BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>

#include <boost/pfr/detail/cast_to_layout_compatible.hpp> // still needed for enums
#include <boost/pfr/detail/offset_based_getter.hpp>
#include <boost/pfr/detail/fields_count.hpp>
#include <boost/pfr/detail/make_flat_tuple_of_references.hpp>
#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/rvalue_t.hpp>


#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif


namespace boost { namespace pfr { namespace detail {

// tag<T,N> generates friend declarations and helps with overload resolution.
// There are two types: one with the auto return type, which is the way we read types later.
// The second one is used in the detection of instantiations without which we'd get multiple
// definitions.

template <class T, std::size_t N>
struct tag {
    friend auto loophole(tag<T,N>);
};

// For returning non default constructible types. Never used at runtime! GCC's std::declval may not be used in potentionally evaluated contexts, so it does not work here.
template <class T> constexpr T& unsafe_declval_like() noexcept {
    T* ptr = nullptr;
    return *ptr;
}

// The definitions of friend functions.
template <class T, class U, std::size_t N, bool B>
struct fn_def {
    friend auto loophole(tag<T,N>) { return boost::pfr::detail::unsafe_declval_like< std::remove_all_extents_t<U> >(); }
};

// This specialization is to avoid multiple definition errors.
template <class T, class U, std::size_t N>
struct fn_def<T, U, N, true> {};


// This has a templated conversion operator which in turn triggers instantiations.
// Important point, using sizeof seems to be more reliable. Also default template
// arguments are "cached" (I think). To fix that I provide a U template parameter to
// the ins functions which do the detection using constexpr friend functions and SFINAE.
template <class T, std::size_t N>
struct loophole_ubiq {
    template<class U, std::size_t M> static std::size_t ins(...);
    template<class U, std::size_t M, std::size_t = sizeof(loophole(tag<T,M>{})) > static char ins(int);

    template<class U, std::size_t = sizeof(fn_def<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
    constexpr operator U&() const noexcept; // `const` here helps to avoid ambiguity in loophole instantiations. optional_like test validate that behavior.
};


// This is a helper to turn a data structure into a tuple.
template <class T, class U>
struct loophole_type_list;

template <typename T, std::size_t... I>
struct loophole_type_list< T, std::index_sequence<I...> >
     // Instantiating loopholes:
    : sequence_tuple::tuple< decltype(T{ loophole_ubiq<T, I>{}... }, 0) >
{
    using type = sequence_tuple::tuple< decltype(loophole(tag<T, I>{}))... >;
};


template <class T>
auto tie_as_tuple_loophole_impl(T& lvalue) noexcept {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    using indexes = std::make_index_sequence<fields_count<type>()>;
    using tuple_type = typename loophole_type_list<type, indexes>::type;

    return boost::pfr::detail::make_flat_tuple_of_references(
        lvalue,
        offset_based_getter<type, tuple_type>{},
        size_t_<0>{},
        size_t_<tuple_type::size_v>{}
    );
}

// Forward declarations:
template <class T> auto tie_as_tuple_recursively(rvalue_t<T> val) noexcept;

template <class T>
auto tie_or_value(T& val, std::enable_if_t<std::is_class< std::remove_reference_t<T> >::value>* = 0) noexcept {
    return boost::pfr::detail::tie_as_tuple_recursively(
        boost::pfr::detail::tie_as_tuple_loophole_impl(val)
    );
}

template <class T>
decltype(auto) tie_or_value(T& val, std::enable_if_t<std::is_enum<std::remove_reference_t<T>>::value>* = 0) noexcept {
    // This is compatible with the pre-loophole implementation, and tests, but IIUC it violates strict aliasing unfortunately
    return cast_to_layout_compatible<
        std::underlying_type_t<std::remove_reference_t<T> >
    >(val);
#if 0
    // This "works", in that it's usable and it doesn't violate strict aliasing.
    // But it means we break compatibility and don't convert enum to underlying type.
    return val;
#endif
}

template <class T>
decltype(auto) tie_or_value(T& val, std::enable_if_t<!std::is_class< std::remove_reference_t<T> >::value && !std::is_enum< std::remove_reference_t<T> >::value>* = 0) noexcept {
    return val;
}

template <class T, std::size_t... I>
auto tie_as_tuple_recursively_impl(T& tup, std::index_sequence<I...> ) noexcept
    ->  sequence_tuple::tuple<
            decltype(boost::pfr::detail::tie_or_value(
                sequence_tuple::get<I>(tup)
            ))...
        >
{
    return {
        boost::pfr::detail::tie_or_value(
            sequence_tuple::get<I>(tup)
        )...
    };
}

template <class T>
auto tie_as_tuple_recursively(rvalue_t<T> tup) noexcept {
    using indexes = std::make_index_sequence<T::size_v>;
    return boost::pfr::detail::tie_as_tuple_recursively_impl(tup, indexes{});
}

template <class T>
auto tie_as_flat_tuple(T& t) {
    auto rec_tuples = boost::pfr::detail::tie_as_tuple_recursively(
        boost::pfr::detail::tie_as_tuple_loophole_impl(t)
    );

    return boost::pfr::detail::make_flat_tuple_of_references(
        rec_tuples, sequence_tuple_getter{}, size_t_<0>{}, size_t_<decltype(rec_tuples)::size_v>{}
    );
}


#if !BOOST_PFR_USE_CPP17
template <class T>
auto tie_as_tuple(T& val) noexcept {
    return boost::pfr::detail::tie_as_tuple_loophole_impl(
        val
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher(T& t, F&& f, std::index_sequence<I...>) {
    std::forward<F>(f)(
        boost::pfr::detail::tie_as_tuple_loophole_impl(t)
    );
}

#endif // #if !BOOST_PFR_USE_CPP17

}}} // namespace boost::pfr::detail


#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif


#endif // BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP

