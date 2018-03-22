// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_FIELDS_COUNT_HPP
#define BOOST_PFR_DETAIL_FIELDS_COUNT_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace boost { namespace pfr { namespace detail {

///////////////////// General utility stuff
template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index >;

///////////////////// Structure that can be converted to reference to anything
struct ubiq_constructor {
    std::size_t ignore;
    template <class Type> constexpr operator Type&() const noexcept; // Undefined, allows initialization of reference fields (T& and const T&)
    //template <class Type> constexpr operator Type&&() const noexcept; // Undefined, allows initialization of rvalue reference fields and move-only types
};

///////////////////// Structure that can be converted to reference to anything except reference to T
template <class T>
struct ubiq_constructor_except {
    template <class Type> constexpr operator std::enable_if_t<!std::is_same<T, Type>::value, Type&> () const noexcept; // Undefined
};

///////////////////// Hand-made is_aggregate_initializable_n<T> trait

// `std::is_constructible<T, ubiq_constructor_except<T>>` consumes a lot of time, so we made a separate lazy trait for it.
template <std::size_t N, class T> struct is_single_field_and_aggregate_initializable: std::false_type {};
template <class T> struct is_single_field_and_aggregate_initializable<1, T>: std::integral_constant<
    bool, !std::is_constructible<T, ubiq_constructor_except<T>>::value
> {};

// Hand-made is_aggregate<T> trait:
// Aggregates could be constructed from `decltype(ubiq_constructor{I})...` but report that there's no constructor from `decltype(ubiq_constructor{I})...`
// Special case for N == 1: `std::is_constructible<T, ubiq_constructor>` returns true if N == 1 and T is copy/move constructible.
template <class T, std::size_t N>
struct is_aggregate_initializable_n {
    template <std::size_t ...I>
    static constexpr bool is_not_constructible_n(std::index_sequence<I...>) noexcept {
        return !std::is_constructible<T, decltype(ubiq_constructor{I})...>::value
            || is_single_field_and_aggregate_initializable<N, T>::value
        ;
    }

    static constexpr bool value =
           std::is_empty<T>::value
        || std::is_fundamental<T>::value
        || is_not_constructible_n(std::make_index_sequence<N>{})
    ;
};

///////////////////// Helper for SFINAE on fields count
template <class T, std::size_t... I>
constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept
    -> typename std::add_pointer<decltype(T{ ubiq_constructor{I}... })>::type;

template <class T, std::size_t N, class /*Enable*/ = decltype( enable_if_constructible_helper<T>(std::make_index_sequence<N>()) ) >
using enable_if_constructible_helper_t = std::size_t;

///////////////////// Non greedy fields count search. Templates instantiation depth is log(sizeof(T)), templates instantiation count is log(sizeof(T)).
template <class T, std::size_t N>
constexpr std::size_t detect_fields_count(size_t_<N>, size_t_<N>, long) noexcept {
    return N;
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(size_t_<Begin>, size_t_<Middle>, int) noexcept;

template <class T, std::size_t Begin, std::size_t Middle>
constexpr auto detect_fields_count(size_t_<Begin>, size_t_<Middle>, long) noexcept
    -> enable_if_constructible_helper_t<T, Middle>
{
    using next_t = size_t_<Middle + (Middle - Begin + 1) / 2>;
    return detect_fields_count<T>(size_t_<Middle>{}, next_t{}, 1L);
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(size_t_<Begin>, size_t_<Middle>, int) noexcept {
    using next_t = size_t_<(Begin + Middle) / 2>;
    return detect_fields_count<T>(size_t_<Begin>{}, next_t{}, 1L);
}

///////////////////// Greedy search. Templates instantiation depth is log(sizeof(T)), templates instantiation count is log(sizeof(T))*T in worst case.
template <class T, std::size_t N>
constexpr auto detect_fields_count_greedy_remember(size_t_<N>, long) noexcept
    -> enable_if_constructible_helper_t<T, N>
{
    return N;
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_greedy_remember(size_t_<N>, int) noexcept {
    return 0;
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_greedy(size_t_<N>, size_t_<N>) noexcept {
    return detect_fields_count_greedy_remember<T>(size_t_<N>{}, 1L);
}

template <class T, std::size_t Begin, std::size_t Last>
constexpr std::size_t detect_fields_count_greedy(size_t_<Begin>, size_t_<Last>) noexcept {
    constexpr std::size_t middle = Begin + (Last - Begin) / 2;
    constexpr std::size_t fields_count_big = detect_fields_count_greedy<T>(size_t_<middle + 1>{}, size_t_<Last>{});
    constexpr std::size_t fields_count_small = detect_fields_count_greedy<T>(size_t_<Begin>{}, size_t_<fields_count_big ? Begin : middle>{});
    return fields_count_big ? fields_count_big : fields_count_small;
}

///////////////////// Choosing between greedy and non greedy search.
template <class T, std::size_t N>
constexpr auto detect_fields_count_dispatch(size_t_<N>, long) noexcept
    -> decltype(sizeof(T{}))
{
    return detect_fields_count<T>(size_t_<0>{}, size_t_<N / 2 + 1>{}, 1L);
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_dispatch(size_t_<N>, int) noexcept {
    // T is not default aggregate initialzable. It means that at least one of the members is not default constructible,
    // so we have to check all the aggregate initializations for T up to N parameters and return the bigest succeeded
    // (we can not use binary search for detecting fields count).
    return detect_fields_count_greedy<T>(size_t_<0>{}, size_t_<N>{});
}

///////////////////// Returns non-flattened fields count
template <class T>
constexpr std::size_t fields_count() noexcept {
    using type = std::remove_cv_t<T>;

    static_assert(
        !std::is_reference<type>::value,
        "Attempt to get fields count on a reference. This is not allowed because that could hide an issue and different library users expect different behavior in that case."
    );

    static_assert(
        std::is_copy_constructible<std::remove_all_extents_t<type>>::value,
        "Type and each field in the type must be copy constructible."
    );

    static_assert(
        !std::is_polymorphic<type>::value,
        "Type must have no virtual function, because otherwise it is not aggregate initializable."
    );

#ifdef __cpp_lib_is_aggregate
    static_assert(
        std::is_aggregate<type>::value             // Does not return `true` for build in types.
        || std::is_standard_layout<type>::value,   // Does not return `true` for structs that have non standard layout members.
        "Type must be aggregate initializable."
    );
#endif

// Can't use the following. See the non_std_layout.cpp test.
//#if !BOOST_PFR_USE_CPP17
//    static_assert(
//        std::is_standard_layout<type>::value,   // Does not return `true` for structs that have non standard layout members.
//        "Type must be aggregate initializable."
//    );
//#endif

    constexpr std::size_t max_fields_count = (sizeof(type) * 8); // We multiply by 8 because we may have bitfields in T
    constexpr std::size_t result = detect_fields_count_dispatch<type>(size_t_<max_fields_count>{}, 1L);

    static_assert(
        is_aggregate_initializable_n<type, result>::value,
        "Types with user specified constructors (non-aggregate initializable types) are not supported."
    );

    static_assert(
        result != 0 || std::is_empty<type>::value || std::is_fundamental<type>::value || std::is_reference<type>::value,
        "Something went wrong. Please report this issue to the github along with the structure you're reflecting."
    );

    return result;
}

}}} // namespace boost::pfr::detail

#ifdef __clang__
#   pragma clang diagnostic pop
#endif

#endif // BOOST_PFR_DETAIL_FIELDS_COUNT_HPP
